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
 * @factory.h
 * The Factory class
 */

#ifndef SPLASH_FACTORY_H
#define SPLASH_FACTORY_H

#include <memory>
#include <vector>

#include "./basetypes.h"

namespace Splash
{

/*************/
//! Factory class, in charge of creating objects base on their type
class Factory
{
  public:
    /**
     * \brief Constructor
     */
    Factory();

    /**
     * \brief Constructor
     * \param root Root object
     */
    Factory(std::weak_ptr<RootObject> root);

    /**
     * \brief Creates a BaseObject given its type
     * \param type Object type
     * \return Return a shared pointer to the created object
     */
    std::shared_ptr<BaseObject> create(const std::string& type);

    /**
     * \brief Get all creatable object types
     * \return Return a vector of all the creatable objects
     */
    std::vector<std::string> getObjectTypes();

    /**
     * \brief Get all objects of the given BaseObject::Category
     * \param c Category to look for
     * \return Return a list of object types of the given class
     */
    std::vector<std::string> getObjectsOfCategory(BaseObject::Category c);

    /**
     * \brief Get object type short description
     * \param type Object type
     * \return Return short description
     */
    std::string getShortDescription(const std::string& type);

    /**
     * \brief Get object type description
     * \param type Object type
     * \return Return description
     */
    std::string getDescription(const std::string& type);

    /**
     * \brief Check whether a type exists
     * \param type Type name
     * \return Return true if the type exists
     */
    bool isCreatable(std::string type);

  private:
    using BuildFuncT = std::function<std::shared_ptr<BaseObject>()>;

    struct Page
    {
        Page() = default;
        Page(BuildFuncT f, BaseObject::Category o = BaseObject::Category::MISC, const std::string& sd = "none", const std::string& d = "none")
            : builder(f)
            , objectCategory(o)
            , shortDescription(sd)
            , description(d)
        {
        }
        BuildFuncT builder{};
        BaseObject::Category objectCategory{BaseObject::Category::MISC};
        std::string shortDescription{"none"};
        std::string description{"none"};
        Values attributes{};
    };

    std::weak_ptr<RootObject> _root;         //!< Root object, used as root for all created objects
    bool _isScene{false};                    //!< True if the root is a Scene, false if it is a World (or if there is no root)
    bool _isMasterScene{false};              //!< True if the root is the master Scene
    std::map<std::string, Page> _objectBook; //!< List of all creatable objects

    std::unordered_map<std::string, std::unordered_map<std::string, Values>> _defaults{}; //!< Default values

    /**
     * \brief Helper function to convert Json::Value to Splash::Values
     * \param values JSon to be processed
     * \return Return a Values converted from the JSon
     */
    Values jsonToValues(const Json::Value& values);

    /**
     * Load default values from the file set in envvar SPLASH_DEFAULTS_FILE_ENV (set in coretypes.h)
     */
    void loadDefaults();

    /**
     * |brief Registers the available objects inside the _objectBook
     */
    void registerObjects();
};

} // end of namespace

#endif
