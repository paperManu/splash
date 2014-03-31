/*
 * Copyright (C) 2014 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @gui.h
 * The Gui base class
 */

#ifndef GUI_H
#define GUI_H

#define SPLASH_GLV_TEXTCOLOR 1.0, 1.0, 1.0
#define SPLASH_GLV_FONTSIZE 8.0
#define SPLASH_GLV_FONTWIDTH 2.0

#define GLFW_NO_GLU
#define GL_GLEXT_PROTOTYPES

#include "config.h"
#include "coretypes.h"

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <GLFW/glfw3.h>
#include <glv.h>

#include "camera.h"
#include "object.h"
#include "texture.h"
#include "widgets.h"

namespace Splash {

class Scene;
typedef std::weak_ptr<Scene> SceneWeakPtr;

/*************/
class Gui : public BaseObject
{
    public:
        /**
         * Constructor
         */
        Gui(GlWindowPtr w, SceneWeakPtr s);

        /**
         * Destructor
         */
        ~Gui();

        /**
         * No copy constructor, but a move one
         */
        Gui(const Gui&) = delete;
        Gui(Gui&& c)
        {
            _isInitialized = c._isInitialized;
            _window = c._window;
            _fbo = c._fbo;
            _outTexture = c._outTexture;
            _objects = c._objects;
            _scene = c._scene;
            _guiCamera = c._guiCamera;
        }

        /**
         * Get pointers to this camera textures
         */
        TexturePtr getTexture() const {return _outTexture;}

        /**
         * Check wether it is initialized
         */
        bool isInitialized() const {return _isInitialized;}

        /**
         * Forward a key event
         */
        void key(int& key, int& action, int& mods);

        /**
         * Forward mouse events
         */
        void mousePosition(int xpos, int ypos);
        void mouseButton(int btn, int action, int mods);
        void mouseScroll(double xoffset, double yoffset);

        /**
         * Try to link the given BaseObject to this
         */
        bool linkTo(BaseObjectPtr obj);

        /**
         * Render this camera into its textures
         */
        bool render();

        /**
         * Set the resolution of the GUI
         */
        void setOutputSize(int width, int height);

    private:
        bool _isInitialized {false};
        GlWindowPtr _window;
        SceneWeakPtr _scene;

        GLuint _fbo;
        TexturePtr _depthTexture;
        TexturePtr _outTexture;
        std::vector<ObjectPtr> _objects;
        float _width {512}, _height {512};

        // GUI specific camera
        CameraPtr _guiCamera;

        // GLV related attributes
        bool _isVisible {false};
        glv::Style _style;
        glv::GLV _glv;
        GlvTextBox _glvLog;
        GlvTextBox _glvProfile;
        GlvTextBox _glvHelp;
        GlvGlobalView _glvGlobalView;
        GlvGraph _glvGraph;
        GlvControl _glvControl;
        glv::space_t _prevMouseX, _prevMouseY;
        bool _flashBG; // Set to true if the BG is set to all white for all outputs
        
        /**
         * Convert GLFW keys values to GLV
         */
        int glfwToGlvKey(int key);

        /**
         * Initialize GLV
         */
        void initGLV(int width, int height);

        /**
         * Update the content of the GLV widgets
         */
        void updateGLV();

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Gui> GuiPtr;

} // end of namespace


#endif // GUI_H
