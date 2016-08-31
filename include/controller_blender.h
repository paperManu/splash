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
 * @controller_blender.h
 * The Blender class, which handles blending-related commands
 */

#ifndef SPLASH_CONTROLLER_BLENDER_H
#define SPLASH_CONTROLLER_BLENDER_H

#include <string>

#include "./controller.h"

namespace Splash {

class Blender : public ControllerObject
{
    public:
        /**
         * \brief Constructor
         * \param root Root object
         */
        Blender(std::weak_ptr<RootObject> root);

        /**
         * \brief Destructor
         */
        ~Blender();

        /**
         * \brief Update the blending
         */
        void render();

    private:
        bool _isSceneMaster {false}; //!< True if the root Scene is master
        std::string _blendingMode {"none"}; //!< Can be "none", "once" or "continuous"
        bool _computeBlending {false}; //!< If true, compute blending in the next render
        bool _continuousBlending {false}; //!< If true, render does not reset _computeBlending
        bool _blendingComputed {false}; //!< True if the blending has been computed
        
        // Vertex blending variables
        std::mutex _vertexBlendingMutex;
        std::condition_variable _vertexBlendingCondition;
        std::atomic_bool _vertexBlendingReceptionStatus {false};

        /**
         * \brief Register new functors to modify attributes
         */
        void registerAttributes();
};

} // end of namespace

#endif // SPLASH_CONTROLLER_BLENDER_H
