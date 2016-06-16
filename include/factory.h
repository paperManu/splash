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

namespace Splash {

/*************/
class Factory
{
    public:
        /**
         * Constructor
         */
        Factory();
        Factory(std::weak_ptr<RootObject> root);

        /**
         * Creates a BaseObject given its type
         */
        std::shared_ptr<BaseObject> create(std::string type);

        /**
         * Get all creatable object types
         */
        std::vector<std::string> getObjectTypes();

    private:
        std::weak_ptr<RootObject> _root;
        bool _isScene {false};
        std::map<std::string, std::function<std::shared_ptr<BaseObject>()>> _objectBook;

        /**
         * Registers the available objects inside the _objectBook
         */
        void registerObjects();
};

} // end of namespace

#endif
