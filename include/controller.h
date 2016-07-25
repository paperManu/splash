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

#include "./coretypes.h"
#include "./basetypes.h"

namespace Splash {

/*************/
class ControllerObject : public BaseObject
{
    public:
        ControllerObject(std::weak_ptr<RootObject> root)
            : BaseObject(root) {}

        virtual ~ControllerObject() {};

    protected:
        /**
         * Get a list of the object names
         */
        std::vector<std::string> getObjectNames() const;

        /**
         * Get the description for the given attribute
         */
        Values getObjectAttributeDescription(const std::string& name, const std::string& attr) const;

        /**
         * Get one specific attribute from the given object
         */
        Values getObjectAttribute(const std::string& name, const std::string& attr) const;

        /**
         * Get all the attributes from the given object
         */
        std::unordered_map<std::string, Values> getObjectAttributes(const std::string& name) const;

        /**
         * Get the links between all objects
         */
        std::unordered_map<std::string, std::vector<std::string>> getObjectLinks() const;

        /**
         * Get a map of the objects types
         */
        std::map<std::string, std::string> getObjectTypes() const;

        /**
         * Get all object of given type
         */
        std::list<std::shared_ptr<BaseObject>> getObjectsOfType(const std::string& type) const;

        /**
         * Set the given configuration-related attribute
         */
        void setGlobal(const std::string& name, const Values& values = {}) const;

        /**
         * Set the given attribute for the given object
         */
        void setObject(const std::string& name, const std::string& attr, const Values& values = {}) const;

        /**
         * Set the given attribute for all objets of the given type
         */
        void setObjectsOfType(const std::string& type, const std::string& attr, const Values& values = {}) const;
};

} // end of namespace

#endif // SPLASH_CONTROLLER_H
