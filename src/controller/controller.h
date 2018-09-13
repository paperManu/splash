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

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./core/graph_object.h"
#include "./core/scene.h"
#include "./userinput/userinput.h"

namespace Splash
{

/*************/
class ControllerObject : public GraphObject
{
  public:
    /**
     * \brief Constructor
     * \param root RootObject
     */
    explicit ControllerObject(RootObject* root)
        : GraphObject(root)
    {
        registerAttributes();
    }

    /**
     * \brief Desctructor
     */
    virtual ~ControllerObject() override {}

    /**
     * Get a ptr to the named object
     * \return The object
     */
    std::shared_ptr<GraphObject> getObjectPtr(const std::string& name) const;

    /**
     * Get a list of pointers to the given objects, if they exist
     * \return The list of pointers
     */
    std::vector<std::shared_ptr<GraphObject>> getObjectsPtr(const std::vector<std::string>& names) const;

    /**
     * Get the alias for the given object
     * \return Return the alias
     */
    std::string getObjectAlias(const std::string& name) const;

    /**
     * Get the aliases for all objects
     * \return Return a map of the aliases
     */
    std::unordered_map<std::string, std::string> getObjectAliases() const;

    /**
     * \brief Get a list of the object names
     * \return Return a vector of all the objects
     */
    std::vector<std::string> getAllObjects() const;

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
     * \return Return an unordered_map of the links, from one object to potentially many others
     */
    std::unordered_map<std::string, std::vector<std::string>> getObjectLinks() const;

    /**
     * \brief Get the reversed links between all objects, from children to parents
     * \return Return an unordered_map the links, from one object to potentially many others
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
    std::vector<std::string> getTypesFromCategory(const GraphObject::Category& category) const;

    /**
     * \brief Get all object of given type.
     * \param type Type to look for. If empty, get all objects.
     * \return Return a list of all objects of the given type
     */
    std::vector<std::string> getObjectsOfType(const std::string& type) const;

    /**
     * Get all objects of the given base type, including derived types
     * \return Return a list of objects of the given type
     */
    template<typename T>
    std::vector<std::shared_ptr<T>> getObjectsOfBaseType() const;

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
    void setWorldAttribute(const std::string& name, const Values& values = {}) const;

    /**
     * Set the given scene-related attribute
     * \param name Attribute name
     * \param values Value to set the attribute to
     */
    void setInScene(const std::string& name, const Values& values = {}) const;

    /**
     * Get the given configuration-related attribute
     * \param attr Attribute name
     * \return Return the attribute value
     */
    Values getWorldAttribute(const std::string& attr) const;

    /**
     * \brief Set the given attribute for the given object
     * \param name Object name
     * \param attr Attribute name
     * \param values Value to set the attribute to
     */
    void setObjectAttribute(const std::string& name, const std::string& attr, const Values& values = {}) const;

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
    void registerAttributes() { GraphObject::registerAttributes(); }
};

/*************/
template<typename T>
std::vector<std::shared_ptr<T>> ControllerObject::getObjectsOfBaseType() const
{
    auto scene = dynamic_cast<Scene*>(_root);
    if (!scene)
        return {};

    auto objects = std::vector<std::shared_ptr<T>>();
    for (auto& obj : scene->_objects)
    {
        auto objAsTexture = std::dynamic_pointer_cast<T>(obj.second);
        if (objAsTexture)
            objects.push_back(objAsTexture);
    }

    return objects;
}

} // end of namespace

#endif // SPLASH_CONTROLLER_H
