/*
 * Copyright (C) 2015 Emmanuel Durand
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
 * @ltcclock.h
 * The LtcClock class, which reads a clock on an audio input
 */

#ifndef SPLASH_LTCCLOCK_H
#define SPLASH_LTCCLOCK_H

#include <ltc.h>
#include <portaudio.h>

#include "config.h"
#include "coretypes.h"
#include "basetypes.h"

namespace Splash {

/*************/
class LtcClock : public BaseObject
{
    public:
        LtcClock();
        ~LtcClock();

    private:
        LTCDecoder* _ltcDecoder {nullptr};
        LTCFrameExt _ltcFrame;

        PaStream* _paStream {nullptr};
};

} // end of namespace

#endif
