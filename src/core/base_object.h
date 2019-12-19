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
#include <optional>
#include <unordered_map>

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./utils/dense_map.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
class BaseObject : public std::enable_shared_from_this<BaseObject>
{
  public:
    /**
     * Constructor.
     */
    BaseObject() { registerAttributes(); }

    /**
     * Destructor.
     */
    virtual ~BaseObject() {}

    /**
     * Set the name of the object.
     * \param name name of the object.
     */
    virtual void setName(const std::string& name) { _name = name; }

    /**
     * Get the name of the object.
     * \return Returns the name of the object.
     */
    inline std::string getName() const { return _name; }

    /**
     * Check whether the object has an attribute of the given name
     * \param name Attribute name
     * \return Return true if the attribute exists
     */
    bool hasAttribute(const std::string& name) const;

    /**
     * Set the specified attribute
     * \param attrib Attribute name
     * \param args Values object which holds attribute values
     * \return Returns true if the parameter exists and was set
     */
    bool setAttribute(const std::string& attrib, const Values& args = {});

    /**
     * Get the specified attribute
     * \param attrib Attribute name
     * \param args Values object which will hold the attribute values
     * \return Return true if the parameter exists
     */
    bool getAttribute(const std::string& attrib, Values& args) const;
    std::optional<Values> getAttribute(const std::string& attrib) const;

    /**
     * Get a list of the object attributes
     * \return Returns a vector holding all the attributes
     */
    std::vector<std::string> getAttributesList() const;

    /**
     * Get the description for the given attribute, if it exists
     * \param name Name of the attribute
     * \return Returns the description for the attribute
     */
    std::string getAttributeDescription(const std::string& name) const;

    /**
     * Get a Values holding the description of all of this object's attributes
     * \return Returns all the descriptions as a Values
     */
    Values getAttributesDescriptions() const;

    /**
     * Get the attribute synchronization method
     * \param name Attribute name
     * \return Return the synchronization method
     */
    Attribute::Sync getAttributeSyncMethod(const std::string& name);

    /**
     * Register a callback to any call to the setter
     * \param attr Attribute to add a callback to
     * \param cb Callback function
     * \return Return a callback handle
     */
    CallbackHandle registerCallback(const std::string& attr, Attribute::Callback cb);

    /**
     * Unregister a callback
     * \param handle A handle to the callback to remove
     * \return True if the callback has been successfully removed
     */
    bool unregisterCallback(const CallbackHandle& handle);

    /**
     * Run the tasks waiting in the object's queue
     */
    void runTasks();

  protected:
    std::string _name{""};                             //!< Object name
    DenseMap<std::string, Attribute> _attribFunctions; //!< Map of all attributes
    mutable std::recursive_mutex _attribMutex;
    bool _updatedParams{true};                         //!< True if the parameters have been updated and the object needs to reflect these changes

    uint32_t _nextAsyncTaskId{0};
    std::map<uint32_t, std::future<void>> _asyncTasks{};
    std::mutex _asyncTaskMutex{};

    std::list<std::function<void()>> _taskQueue;
    std::recursive_mutex _taskMutex;

    struct PeriodicTask
    {
        PeriodicTask(std::function<void()> func, uint32_t period)
            : task(func)
            , period(period)
        {
        }
        std::function<void()> task{};
        uint32_t period{0};
        int64_t lastCall{0};
    };
    std::map<std::string, PeriodicTask> _periodicTasks{};
    std::mutex _periodicTaskMutex{};

    /**
     * Add a new task to the queue
     * \param task Task function
     */
    void addTask(const std::function<void()>& task);

    /**
     * Add a task repeated at each frame
     * Note that the period is not a hard constraint, and depends on the framerate
     * \param name Task name
     * \param task Task function
     * \param period Delay (in ms) between each call. If 0, it will be called at each frame
     */
    void addPeriodicTask(const std::string& name, const std::function<void()>& task, uint32_t period = 0);

    /**
     * Add a new attribute to this object
     * \param name Attribute name
     * \param set Set function
     * \param types Vector of char holding the expected parameters for the set function
     * \return Return a reference to the created attribute
     */
    virtual Attribute& addAttribute(const std::string& name, const std::function<bool(const Values&)>& set, const std::vector<char>& types = {});

    /**
     * Add a new attribute to this object
     * \param name Attribute name
     * \param set Set function
     * \param get Get function
     * \param types Vector of char holding the expected parameters for the set function
     * \return Return a reference to the created attribute
     */
    virtual Attribute& addAttribute(
        const std::string& name, const std::function<bool(const Values&)>& set, const std::function<const Values()>& get, const std::vector<char>& types = {});

    /**
     * Run a task asynchronously
     * \param func Function to run
     */
    void runAsyncTask(const std::function<void(void)>& func);

    /**
     * Set and the description for the given attribute, if it exists
     * \param name Attribute name
     * \param description Attribute description
     */
    void setAttributeDescription(const std::string& name, const std::string& description);

    /**
     * Set attribute synchronization method
     * \param Method Synchronization method, can be any of the Attribute::Sync values
     */
    void setAttributeSyncMethod(const std::string& name, const Attribute::Sync& method);

    /**
     * Remove the specified attribute
     * \param name Attribute name
     */
    void removeAttribute(const std::string& name);

    /**
     * Remove a periodic task
     * \param name Task name
     */
    void removePeriodicTask(const std::string& name);

    /**
     * Register new attributes
     */
    void registerAttributes() {}
};

} // namespace Splash

#endif // SPLASH_BASE_OBJECT_H
