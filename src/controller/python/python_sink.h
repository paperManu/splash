/*
 * Copyright (C) 2018 Emmanuel Durand
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
 * @python_sink.h
 * The PythonSink class, which embeds Splash Sinks
 */

#ifndef SPLASH_PYTHON_SINK_H
#define SPLASH_PYTHON_SINK_H

#include <atomic>
#include <memory>

#include <Python.h>

#include "./sink/sink.h"

namespace Splash
{

/*************/
class PythonSink
{
  public:
    struct PythonSinkObject
    {
        static std::atomic_int sinkIndex;
        PyObject_HEAD std::string sourceName{""};
        uint32_t width{512};
        uint32_t height{512};
        bool keepRatio{false};
        uint32_t framerate{30};
        std::string sinkName{""};
        std::string filterName{""};
        std::shared_ptr<Splash::Sink> sink{nullptr};
        bool linked{false};
        bool opened{false};
        PyObject* lastBuffer{nullptr};
    };
    PythonSinkObject pythonSinkObject;

    // Sink wrapper methods. They are in this class to be able to access the Splash capsule
    static void pythonSinkDealloc(PythonSinkObject* self);
    static PyObject* pythonSinkNew(PyTypeObject* type, PyObject* args, PyObject* kwds);
    static int pythonSinkInit(PythonSinkObject* self, PyObject* args, PyObject* kwds);
    static PyObject* pythonSinkLink(PythonSinkObject* self, PyObject* args, PyObject* kwds);
    static PyObject* pythonSinkUnlink(PythonSinkObject* self);
    static PyObject* pythonSinkGrab(PythonSinkObject* self);
    static PyObject* pythonSinkSetSize(PythonSinkObject* self, PyObject* args, PyObject* kwds);
    static PyObject* pythonSinkGetSize(PythonSinkObject* self);
    static PyObject* pythonSinkKeepRatio(PythonSinkObject* self, PyObject* args, PyObject* kwds);
    static PyObject* pythonSinkSetFramerate(PythonSinkObject* self, PyObject* args, PyObject* kwds);
    static PyObject* pythonSinkOpen(PythonSinkObject* self);
    static PyObject* pythonSinkClose(PythonSinkObject* self);
    static PyObject* pythonSinkGetCaps(PythonSinkObject* self);

    static PyMethodDef SinkMethods[];
    static PyTypeObject pythonSinkType;
};

}

#endif // SPLASH_PYTHON_SINK_H
