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
 * @widget_global_view.h
 * The global view widget, to calibrate cameras
 */

#ifndef SPLASH_WIDGET_MEDIA_H
#define SPLASH_WIDGET_MEDIA_H

#include "./widget.h"

namespace Splash
{

/*************/
class GuiMedia : public GuiWidget
{
    public:
        GuiMedia(std::weak_ptr<Scene> scene, std::string name);
        void render();
        int updateWindowFlags();

    private:
        std::map<std::string, int> _mediaTypeIndex;
        std::map<std::string, std::string> _mediaTypes {{"image", "image"},
                                                        {"video", "image_ffmpeg"},
                                                        {"shared memory", "image_shmdata"},
                                                        {"queue", "queue"},
#if HAVE_OPENCV
                                                        {"video grabber", "image_opencv"},
#endif
#if HAVE_OSX
                                                        {"syphon", "texture_syphon"},
#endif
                                                        };
        std::map<std::string, std::string> _mediaTypesReversed {}; // Created from the previous map

        Values _newMedia {"image", "", 0.f, 0.f};
        int _newMediaTypeIndex {0};
        float _newMediaStart {0.f};
        float _newMediaStop {0.f};

        std::list<std::shared_ptr<BaseObject>> getSceneMedia();
        std::list<std::shared_ptr<BaseObject>> getFiltersForImage(const std::shared_ptr<BaseObject>& image);
        void replaceMedia(std::string previousMedia, std::string type);
};

} // end of namespace

#endif
