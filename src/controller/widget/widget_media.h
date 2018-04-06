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
 * @widget_media.h
 * The media widget, to modify the input media type
 */

#ifndef SPLASH_WIDGET_MEDIA_H
#define SPLASH_WIDGET_MEDIA_H

#include "./controller/widget/widget.h"

namespace Splash
{

/*************/
class GuiMedia : public GuiWidget
{
  public:
    GuiMedia(Scene* scene, const std::string& name);
    void render() final;
    int updateWindowFlags() final;

  private:
    std::map<std::string, int> _mediaTypeIndex;
    std::map<std::string, std::string> _mediaTypes;
    std::map<std::string, std::string> _mediaTypesReversed{}; // Created from the previous map

    Values _newMedia{"", "", 0.f, 0.f, 0};
    int _newMediaTypeIndex{0};
    float _newMediaStart{0.f};
    float _newMediaStop{0.f};
    bool _newMediaFreeRun{false};

    std::list<std::shared_ptr<BaseObject>> getSceneMedia();
    std::list<std::shared_ptr<BaseObject>> getFiltersForImage(const std::shared_ptr<BaseObject>& image);
    void replaceMedia(const std::string& previousMedia, const std::string& type);
};

} // end of namespace

#endif
