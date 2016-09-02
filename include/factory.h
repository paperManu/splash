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
    std::shared_ptr<BaseObject> create(std::string type);

    /**
     * \brief Get all creatable object types
     * \return Return a vector of all the creatable objects
     */
    std::vector<std::string> getObjectTypes();

  private:
    std::weak_ptr<RootObject> _root;                                                 //!< Root object, used as root for all created objects
    bool _isScene{false};                                                            //!< True if the root is a Scene, false if it is a World (or if there is no root)
    std::map<std::string, std::function<std::shared_ptr<BaseObject>()>> _objectBook; //!< List of all creatable objects

    /**
     * |brief Registers the available objects inside the _objectBook
     */
    void registerObjects();
};

} // end of namespace

#endif
