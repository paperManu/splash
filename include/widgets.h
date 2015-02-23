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
 * @widgets.h
 * The splash widgets
 */

#ifndef SPLASH_WIDGETS_H
#define SPLASH_WIDGETS_H

#define SPLASH_GLV_TEXTCOLOR 1.0, 1.0, 1.0
#define SPLASH_GLV_FONTSIZE 8.0
#define SPLASH_GLV_FONTWIDTH 2.0

#include "config.h"
#include "coretypes.h"
#include "basetypes.h"

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <glv.h>

#include "camera.h"
#include "object.h"
#include "texture.h"

namespace Splash
{
class Gui;
class Scene;
typedef std::weak_ptr<Scene> SceneWeakPtr;

/*************/
class GuiWidget
{
    public:
        GuiWidget(std::string name = "");
        virtual ~GuiWidget() {}
        virtual void render() {}

    protected:
        std::string _name {""};
};

/*************/
class GuiTextBox : public GuiWidget
{
    public:
        GuiTextBox(std::string name = "");
        void render();
        void setTextFunc(std::function<std::string()> func) {getText = func;}

    private:
        std::function<std::string()> getText;
};

/*************/
class GuiGlobalView : public GuiWidget
{
    public:
        GuiGlobalView(std::string name = "");
        void render();
        void setScene(SceneWeakPtr scene) {_scene = scene;}
        void setCamera(CameraPtr cam);
        void SetObject(ObjectPtr obj) {_camera->linkTo(obj);}

    protected:
        CameraPtr _camera, _guiCamera;
        SceneWeakPtr _scene;
        bool _camerasHidden {false};
        bool _beginDrag {true};
        bool _noMove {false};

        // Size of the view
        int _camWidth, _camHeight;

        // Store the previous camera values
        Values _eye, _target, _up, _fov, _principalPoint;
        Values _newTarget;

        // Previous point added
        Values _previousPointAdded;

        void processKeyEvents();
        void processMouseEvents();
};

/*************/
class GlvControl : public glv::View
{
    public:
        GlvControl();
        void onDraw(glv::GLV& g);
        bool onEvent(glv::Event::t e, glv::GLV& g);
        void setScene(SceneWeakPtr scene) {_scene = scene;}

    private:
        SceneWeakPtr _scene;
        bool _ready {false};

        int _objIndex {0};
        bool _isDistant {false};
        glv::Label _selectedObjectName;
        glv::View _left, _right;
        glv::Placer _titlePlacer;

        std::vector<glv::Label*> _properties;
        std::vector<glv::NumberDialer*> _numbers;

        std::string getNameByIndex();
        void changeTarget(std::string name);
};

/*************/
class GuiGraph : public GuiWidget
{
    public:
        GuiGraph(std::string name) : GuiWidget(name) {}
        void render();

    private:
        std::atomic_uint _target;
        unsigned int _maxHistoryLength {500};
        std::map<std::string, std::deque<unsigned long long>> _durationGraph;
};

} // end of namespace

#endif // SPLASH_WIDGETS_H
