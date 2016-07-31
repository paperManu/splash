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
 * @pythonEmbedded.h
 * The PythonEmbedded class, which runs a Python 3.x controller
 */

#ifndef SPLASH_PYTHON_EMBEDDED_H
#define SPLASH_PYTHON_EMBEDDED_H

#include <atomic>
#include <future>
#include <string>
#include <thread>

#include <Python.h>

#include "./coretypes.h"
#include "./basetypes.h"
#include "./controller.h"

namespace Splash {

/*************/
class PythonEmbedded : public ControllerObject
{
    public:
        /**
         * \brief Constructor
         */
        PythonEmbedded(std::weak_ptr<RootObject> root);

        /**
         * \brief Destructor
         */
        ~PythonEmbedded();

        /**
         * \brief Set the path to the source Python file
         * \param filepath Path to the source file
         * \return Return true if the file exists
         */
        bool setScriptFile(const std::string& filepath);

        /**
         * \brief Run the script
         * \return Return true if the script launched successfully
         */
        bool run();

        /**
         * \brief Stop the running script
         */
        void stop();

    private:
        std::string _filepath {""}; //!< Path to the python script
        std::string _scriptName {""}; //!< Name of the module (filename minus .py)

        static std::atomic_int _pythonInstances; //!< Number of Python scripts running
        bool _doLoop {false}; //!< Set to false to stop the Python loop
        int _loopDurationMs {5}; //!< Time between loops in ms
        std::thread _loopThread {}; //!< Python thread loop
        std::promise<bool> _loopThreadPromise {}; //!< Holds the output result from the threading loop

        std::vector<PyMethodDef> _pythonMethods {};

        /**
         * \brief Python interpreter main loop
         */
        void loop();

        /**
         * \brief Get a Python function from the given module
         * \param module Python module
         * \param name Function name
         * \return Return a python function, or nullptr if it does not exist
         */
        PyObject* getFuncFromModule(PyObject* module, const std::string& name);

        /**
         * \brief Build a Python object from a Value
         * \param value Value to convert
         * \return Return a PyObject* representation of the value
         */
        static PyObject* convertFromValue(const Value& value);

        /**
         * \brief Build a Value from a valid Python object
         * \param pyObject Python object to interpret
         * \return Return a Value
         */
        static Value convertToValue(PyObject* pyObject);

        /**
         * \brief Register new functors to modify attributes
         */
        void registerAttributes();

    private:
        static std::mutex _pythonMutex;
        static std::unique_ptr<ControllerObject> _controller;
        static PyThreadState* _pythonGlobalThreadState;

        // Python objects and methods
        static PyMethodDef SplashMethods[];
        static PyModuleDef SplashModule;

        static PyObject* pythonInitSplash();
        static PythonEmbedded* getSplashInstance(PyObject* module);
        static PyObject* pythonGetObjectList(PyObject* self, PyObject* args);
        static PyObject* pythonGetObjectTypes(PyObject* self, PyObject* args);
        static PyObject* pythonGetObjectAttributeDescription(PyObject* self, PyObject* args);
        static PyObject* pythonGetObjectAttribute(PyObject* self, PyObject* args);
        static PyObject* pythonGetObjectAttributes(PyObject* self, PyObject* args);
        static PyObject* pythonGetObjectLinks(PyObject* self, PyObject* args);
        static PyObject* pythonSetGlobal(PyObject* self, PyObject* args);
        static PyObject* pythonSetObject(PyObject* self, PyObject* args);
        static PyObject* pythonSetObjectsOfType(PyObject* self, PyObject* args);
};

} // end of namespace

#endif // SPLASH_PYTHON_EMBEDDED_H
