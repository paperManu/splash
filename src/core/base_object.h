/*
 * Copyright (C) 2017 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @base_object.h
 * Base type for all objects, from which more complex classes derive
 */

#ifndef SPLASH_BASE_OBJECT_H
#define SPLASH_BASE_OBJECT_H

#include <atomic>
#include <condition_variable>
#include <future>
#include <json/json.h>
#include <list>
#include <map>
#include <unordered_map>

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
class BaseObject
{
  public:
    /**
     * \brief Constructor.
     */
    BaseObject() { registerAttributes(); }

    /**
     * \brief Destructor.
     */
    virtual ~BaseObject() {}

    /**
     * \brief Set the name of the object.
     * \param name name of the object.
     */
    inline void setName(const std::string& name) { _name = name; }

    /**
     * \brief Get the name of the object.
     * \return Returns the name of the object.
     */
    inline std::string getName() const { return _name; }

    /**
     * \brief Set the specified attribute
     * \param attrib Attribute name
     * \param args Values object which holds attribute values
     * \return Returns true if the parameter exists and was set
     */
    bool setAttribute(const std::string& attrib, const Values& args = {});

    /**
     * \brief Get the specified attribute
     * \param attrib Attribute name
     * \param args Values object which will hold the attribute values
     * \param includeDistant Return true even if the attribute is distant
     * \param includeNonSavable Return true even if the attribute is not savable
     * \return Return true if the parameter exists and is savable
     */
    bool getAttribute(const std::string& attrib, Values& args, bool includeDistant = false, bool includeNonSavable = false) const;

    /**
     * \brief Get all the savable attributes as a map
     * \param includeDistant Also include the distant attributes
     * \return Return the map of all the attributes
     */
    std::unordered_map<std::string, Values> getAttributes(bool includeDistant = false) const;

    /**
     * \brief Converts a Value as a Json object
     * \param values Value to convert
     * \param asObject If true, return a Json object
     * \return Returns a Json object
     */
    Json::Value getValuesAsJson(const Values& values, bool asObject = false) const;

    /**
     * \brief Get the object's configuration as a Json object
     * \return Returns a Json object
     */
    virtual Json::Value getConfigurationAsJson() const;

    /**
     * \brief Get the description for the given attribute, if it exists
     * \param name Name of the attribute
     * \return Returns the description for the attribute
     */
    std::string getAttributeDescription(const std::string& name);

    /**
     * \brief Get a Values holding the description of all of this object's attributes
     * \return Returns all the descriptions as a Values
     */
    Values getAttributesDescriptions();

    /**
     * \brief Get the attribute synchronization method
     * \param name Attribute name
     * \return Return the synchronization method
     */
    Attribute::Sync getAttributeSyncMethod(const std::string& name);

    /**
     * Run the tasks waiting in the object's queue
     */
    virtual void runTasks();

  protected:
    std::string _name{""};                                              //!< Object name
    std::unordered_map<std::string, Attribute> _attribFunctions; //!< Map of all attributes
    bool _updatedParams{true};                                          //!< True if the parameters have been updated and the object needs to reflect these changes

    std::future<void> _asyncTask{};
    std::mutex _asyncTaskMutex{};

    std::list<std::function<void()>> _taskQueue;
    std::recursive_mutex _taskMutex;

    /**
     * Add a new task to the queue
     * \param task Task function
     */
    void addTask(const std::function<void()>& task);

    /**
     * \brief Add a new attribute to this object
     * \param name Attribute name
     * \param set Set function
     * \param types Vector of char holding the expected parameters for the set function
     * \return Return a reference to the created attribute
     */
    Attribute& addAttribute(const std::string& name, const std::function<bool(const Values&)>& set, const std::vector<char>& types = {});

    /**
     * \brief Add a new attribute to this object
     * \param name Attribute name
     * \param set Set function
     * \param get Get function
     * \param types Vector of char holding the expected parameters for the set function
     * \return Return a reference to the created attribute
     */
    Attribute& addAttribute(
        const std::string& name, const std::function<bool(const Values&)>& set, const std::function<const Values()>& get, const std::vector<char>& types = {});

    /**
     * Run a task asynchronously, one task at a time
     * \param func Function to run
     */
    void runAsyncTask(const std::function<void(void)>& func);

    /**
     * \brief Set and the description for the given attribute, if it exists
     * \param name Attribute name
     * \param description Attribute description
     */
    void setAttributeDescription(const std::string& name, const std::string& description);

    /**
     * \brief Set attribute synchronization method
     * \param Method Synchronization method, can be any of the Attribute::Sync values
     */
    void setAttributeSyncMethod(const std::string& name, const Attribute::Sync& method);

    /**
     * \brief Remove the specified attribute
     * \param name Attribute name
     */
    void removeAttribute(const std::string& name);

    /**
     * \brief Set additional parameters for a given attribute
     * \param name Attribute name
     * \param savable Savability
     * \param updateDistant If true and the object has a World as root, updates the attribute of the corresponding Scene object
     */
    void setAttributeParameter(const std::string& name, bool savable, bool updateDistant);

    /**
     * \brief Register new attributes
     */
    void registerAttributes() {}
};

} // namespace Splash

#endif // SPLASH_BASE_OBJECT_H
