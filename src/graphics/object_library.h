/*
 * Copyright (C) 2018 Splash authors
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
 * @object_library.h
 * The ObjectLibrary class, storing 3D models loaded into Objects
 */

#ifndef SPLASH_OBJECT_LIBRARY_H
#define SPLASH_OBJECT_LIBRARY_H

#include <map>
#include <memory>
#include <string>

#include "./graphics/geometry.h"
#include "./graphics/object.h"
#include "./mesh/mesh.h"

namespace Splash
{

class Scene;

/*************/
class ObjectLibrary
{
  public:
    /**
     * Constructor
     */
    explicit ObjectLibrary(Scene* scene);
    ~ObjectLibrary() = default;

    /**
     * Load a 3D model for later use
     * \param name Key to access the generated object
     * \param filename Filename of the 3D model
     * \return Return true if the model has been loaded successfully, false if it already exists or failed loading
     */
    bool loadModel(const std::string& name, const std::string& filename);

    /**
     * Get an object generated from a 3D model given its key name
     * \param name Name of the object
     * \return Return a raw pointer to the Object
     */
    Object* getModel(const std::string& name);

  private:
    Scene* _scene;
    gfx::Renderer* _renderer{nullptr};
    std::unique_ptr<Object> _defaultObject{nullptr};
    std::map<std::string, std::unique_ptr<Object>> _library{};
};

} // namespace Splash

#endif // SPLASH_OBJECT_LIBRARY_H
