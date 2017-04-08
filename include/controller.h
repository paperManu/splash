/*
 * Copyright (C) 2016 Emmanuel Durand
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
 * @controller.h
 * The ControllerObject base class
 */

#ifndef SPLASH_CONTROLLER_H
#define SPLASH_CONTROLLER_H

#include <string>
#include <vector>

#include "./attribute.h"
#include "./coretypes.h"
#include "./userInput.h"

namespace Splash
{

/*************/
class ControllerObject : public BaseObject
{
  public:
    /**
     * \brief Constructor
     * \param root RootObject
     */
    ControllerObject(RootObject* root)
        : BaseObject(root)
    {
        registerAttributes();
    }

    /**
     * \brief Desctructor
     */
    virtual ~ControllerObject() override {}

    /**
     * \brief Get a list of the object names
     * \return Return a vector of all the objects
     */
    std::vector<std::string> getObjectNames() const;

    /**
     * \brief Get the description for the given attribute
     * \param name Object name
     * \param attr Attribute name
     * \return Return the description of the given attribute
     */
    Values getObjectAttributeDescription(const std::string& name, const std::string& attr) const;

    /**
     * \brief Get one specific attribute from the given object
     * \param name Object name
     * \param attr Attribute
     * \return Return the value of the given attribute
     */
    Values getObjectAttribute(const std::string& name, const std::string& attr) const;

    /**
     * \brief Get all the attributes from the given object
     * \param name Object name
     * \return Return a map of all of the object's attributes
     */
    std::unordered_map<std::string, Values> getObjectAttributes(const std::string& name) const;

    /**
     * \brief Get the links between all objects, from parents to children
     * \return Return an unordered_map the links, from one to many
     */
    std::unordered_map<std::string, std::vector<std::string>> getObjectLinks() const;

    /**
     * \brief Get the reversed links between all objects, from children to parents
     * \return Return an unordered_map the links, from one to many
     */
    std::unordered_map<std::string, std::vector<std::string>> getObjectReversedLinks() const;

    /**
     * \brief Get a map of the object types
     * \return Return a map of all object types
     */
    std::map<std::string, std::string> getObjectTypes() const;

    /**
     * \brief Get an object short description
     * \param type Object type
     * \return Return a short description
     */
    std::string getShortDescription(const std::string& type) const;

    /**
     * \brief Get an object description
     * \param type Object type
     * \return Return a description
     */
    std::string getDescription(const std::string& type) const;

    /**
     * \brief Get all types of given category
     * \param category Category to look for
     * \return Return a list of all types of the given category
     */
    std::vector<std::string> getTypesFromCategory(const BaseObject::Category& category) const;

    /**
     * \brief Get all object of given type. If empty, get all objects.
     * \param type Type to look for
     * \return Return a list of all objects of the given type
     */
    std::list<std::shared_ptr<BaseObject>> getObjectsOfType(const std::string& type) const;

    /**
     * \brief Send a serialized buffer to the given BufferObject
     * \param name Object name
     * \param buffer Serialized buffer
     */
    void sendBuffer(const std::string& name, const std::shared_ptr<SerializedObject>& buffer) const;

    /**
     * \brief Set the given configuration-related attribute
     * \param name Attribute name
     * \param values Value to set the attribute to
     */
    void setGlobal(const std::string& name, const Values& values = {}) const;

    /**
     * Get the given configuration-related attribute
     * \param attr Attribute name
     * \return Return the attribute value
     */
    Values getGlobal(const std::string& attr) const;

    /**
     * \brief Set the given attribute for the given object
     * \param name Object name
     * \param attr Attribute name
     * \param values Value to set the attribute to
     */
    void setObject(const std::string& name, const std::string& attr, const Values& values = {}) const;

    /**
     * \brief Set the given attribute for all objets of the given type
     * \param type Object type
     * \param attr Attribute
     * \param values Value to set the attribute to
     */
    void setObjectsOfType(const std::string& type, const std::string& attr, const Values& values = {}) const;

    /**
     * \brief Set a user input callback, to capture a user event
     * \param state State which should trigger the callback
     * \param cb Callback
     */
    void setUserInputCallback(const UserInput::State& state, std::function<void(const UserInput::State&)>& cb) const;

  protected:
    /**
     * \brief Register new functors to modify attributes
     */
    void registerAttributes() { BaseObject::registerAttributes(); }
};

} // end of namespace

#endif // SPLASH_CONTROLLER_H
