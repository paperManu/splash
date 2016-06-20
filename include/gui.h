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
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @gui.h
 * The Gui base class
 */

#ifndef SPLASH_GUI_H
#define SPLASH_GUI_H

#define SPLASH_GLV_TEXTCOLOR 1.0, 1.0, 1.0
#define SPLASH_GLV_FONTSIZE 8.0
#define SPLASH_GLV_FONTWIDTH 2.0

#define GLFW_NO_GLU
#define GL_GLEXT_PROTOTYPES

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <imgui.h>

#include "config.h"

#if HAVE_GPHOTO
    #include "./colorcalibrator.h"
#endif
#include "./coretypes.h"
#include "./basetypes.h"
#include "./widget_control.h"
#include "./widget_global_view.h"
#include "./widget_graph.h"
#include "./widget_media.h"
#include "./widget_node_view.h"
#include "./widget_template.h"
#include "./widget_text_box.h"
#include "./widget_warp.h"

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
        Gui& operator=(const Gui&) = delete;

        /**
         * Get pointers to this camera textures
         */
        TexturePtr getTexture() const {return _outTexture;}

        /**
         * Check wether it is initialized
         */
        bool isInitialized() const {return _isInitialized;}

        /**
         * Forward a unicode char event
         */
        void unicodeChar(unsigned int unicodeChar);

        /**
         * Forward joystick state
         */
        void setJoystick(const std::vector<float>& axes, const std::vector<uint8_t>& buttons);

        /**
         * Forward a key event
         */
        void key(int key, int action, int mods);

        /**
         * Forward mouse events
         */
        void mousePosition(int xpos, int ypos);
        void mouseButton(int btn, int action, int mods);
        void mouseScroll(double xoffset, double yoffset);

        /**
         * Try to link / unlink the given BaseObject to this
         */
        bool linkTo(std::shared_ptr<BaseObject> obj);
        void unlinkFrom(std::shared_ptr<BaseObject> obj);

        /**
         * Render this camera into its textures
         */
        bool render();

        /**
         * Specify the configuration path (as loaded by World)
         */
        void setConfigFilePath(const std::string& path) {_configurationPath = path.data();}

        /**
         * Set the resolution of the GUI
         */
        void setOutputSize(int width, int height);

    private:
        bool _isInitialized {false};
        GlWindowPtr _window;
        SceneWeakPtr _scene;

        GLuint _fbo;
        Texture_ImagePtr _depthTexture;
        TexturePtr _outTexture;
        float _width {512}, _height {512};

        // GUI specific camera
        CameraPtr _guiCamera;

        // ImGUI related attributes
        static GLuint _imFontTextureId;
        static GLuint _imGuiShaderHandle, _imGuiVertHandle, _imGuiFragHandle;
        static GLint _imGuiTextureLocation;
        static GLint _imGuiProjMatrixLocation;
        static GLint _imGuiPositionLocation;
        static GLint _imGuiUVLocation;
        static GLint _imGuiColorLocation;
        static GLuint _imGuiVboHandle, _imGuiElementsHandle, _imGuiVaoHandle;
        static size_t _imGuiVboMaxSize;

        // ImGUI objects
        ImGuiWindowFlags _windowFlags {0};
        std::vector<std::shared_ptr<GuiWidget>> _guiWidgets;

        // Gui related attributes
        std::string _configurationPath;
        bool _isVisible {false};
        bool _flashBG {false}; // Set to true if the BG is set to all white for all outputs
        bool _wireframe {false};
        bool _blendingActive {false};

        /**
         * Initialize ImGui
         */
        void initImGui(int width, int height);
        void initImWidgets();

        /**
         * ImGui render function
         */
        static void imGuiRenderDrawLists(ImDrawData* draw_data);

        /**
         * Actions
         */
        void activateLUT();
        void calibrateColorResponseFunction();
        void calibrateColors();
        void computeBlending(bool once = false);
        void flashBackground();
        void copyCameraParameters();
        void loadConfiguration();
        void saveConfiguration();

        /**
         * Register new functors to modify attributes
         */
        void registerAttributes();
};

typedef std::shared_ptr<Gui> GuiPtr;

} // end of namespace


#endif // SPLASH_GUI_H
