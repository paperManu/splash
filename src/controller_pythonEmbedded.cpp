#include "./controller_pythonEmbedded.h"

#include <functional>
#include <fstream>
#include <mutex>

#include "./osUtils.h"

using namespace std;

namespace Splash {

/*************/
atomic_int PythonEmbedded::_pythonInstances {0};
mutex PythonEmbedded::_pythonMutex {};
unique_ptr<ControllerObject> PythonEmbedded::_controller {nullptr};
PyThreadState* PythonEmbedded::_pythonGlobalThreadState {nullptr};

/*************/
PythonEmbedded* PythonEmbedded::getSplashInstance(PyObject* module)
{
    return static_cast<PythonEmbedded*>(PyCapsule_Import("splash.splash", 0));
}

/*************/
PyObject* PythonEmbedded::pythonGetObjectList(PyObject* self, PyObject* args)
{
    //auto that = getSplashInstance(self);
    if (!_controller)
        return PyList_New(0);

    auto objects = _controller->getObjectNames();
    PyObject* pythonObjectList = PyList_New(objects.size());
    for (int i = 0; i < objects.size(); ++i)
        PyList_SetItem(pythonObjectList, i, Py_BuildValue("s", objects[i].c_str()));

    return pythonObjectList;
}

/*************/
PyObject* PythonEmbedded::pythonGetObjectTypes(PyObject* self, PyObject* args)
{
    //auto that = getSplashInstance(self);
    if (!_controller)
        return PyDict_New();

    auto objects = _controller->getObjectTypes();
    PyObject* pythonObjectDict = PyDict_New();
    for (auto& obj : objects)
    {
        PyObject* val = PyUnicode_FromString(obj.second.c_str());
        PyDict_SetItemString(pythonObjectDict, obj.first.c_str(), val);
        Py_DECREF(val);
    }

    return pythonObjectDict;
}

/*************/
PyObject* PythonEmbedded::pythonGetObjectAttributeDescription(PyObject* self, PyObject* args)
{
    //auto that = getSplashInstance(self);
    if (!_controller)
        return Py_BuildValue("s", "");

    char* strName;
    char* strAttr;
    if (!PyArg_ParseTuple(args, "ss", &strName, &strAttr))
        return Py_BuildValue("s", "");

    auto result = _controller->getObjectAttributeDescription(string(strName), string(strAttr));
    if (result.size() == 0)
        return Py_BuildValue("s", "");

    auto description = result[0].asString();
    return Py_BuildValue("s", description.c_str());
}

/*************/
PyObject* PythonEmbedded::pythonGetObjectAttribute(PyObject* self, PyObject* args)
{
    //auto that = getSplashInstance(self);
    if (!_controller)
        return PyList_New(0);

    char* strName;
    char* strAttr;
    if (!PyArg_ParseTuple(args, "ss", &strName, &strAttr))
        return PyList_New(0);

    auto result = _controller->getObjectAttribute(string(strName), string(strAttr));
    auto pyResult = convertFromValue(result);

    return pyResult;
}

/*************/
PyObject* PythonEmbedded::pythonGetObjectAttributes(PyObject* self, PyObject* args)
{
    //auto that = getSplashInstance(self);
    if (!_controller)
        return PyList_New(0);

    char* strName;
    if (!PyArg_ParseTuple(args, "s", &strName))
        return PyDict_New();

    auto result = _controller->getObjectAttributes(string(strName));
    auto pyResult = PyDict_New();
    for (auto& r : result)
    {
        auto pyValue = convertFromValue(r.second);
        PyDict_SetItemString(pyResult, r.first.c_str(), pyValue);
        Py_DECREF(pyValue);
    }

    return pyResult;
}

/*************/
PyObject* PythonEmbedded::pythonGetObjectLinks(PyObject* self, PyObject* args)
{
    //auto that = getSplashInstance(self);
    if (!_controller)
        return PyDict_New();

    auto objects = _controller->getObjectLinks();
    PyObject* pythonObjectDict = PyDict_New();
    for (auto& obj : objects)
    {
        PyObject* links = PyList_New(obj.second.size());
        for (int i = 0; i < obj.second.size(); ++i)
            PyList_SetItem(links, i, Py_BuildValue("s", obj.second[i].c_str()));
        PyDict_SetItemString(pythonObjectDict, obj.first.c_str(), links);
        Py_DECREF(links);
    }

    return pythonObjectDict;
}

/*************/
PyObject* PythonEmbedded::pythonSetGlobal(PyObject* self, PyObject* args)
{
    //auto that = getSplashInstance(self);
    if (!_controller)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    char* strName;
    PyObject* pyValue;
    if (!PyArg_ParseTuple(args, "sO", &strName, &pyValue))
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto value = convertToValue(pyValue).asValues();
    _controller->setGlobal(string(strName), value);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyObject* PythonEmbedded::pythonSetObject(PyObject* self, PyObject* args)
{
    //auto that = getSplashInstance(self);
    if (!_controller)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    char* strName;
    char* strAttr;
    PyObject* pyValue;
    if (!PyArg_ParseTuple(args, "ssO", &strName, &strAttr, &pyValue))
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto value = convertToValue(pyValue).asValues();
    _controller->setObject(string(strName), string(strAttr), value);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyObject* PythonEmbedded::pythonSetObjectsOfType(PyObject* self, PyObject* args)
{
    //auto that = getSplashInstance(self);
    if (!_controller)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    char* strType;
    char* strAttr;
    PyObject* pyValue;
    if (!PyArg_ParseTuple(args, "ssO", &strType, &strAttr, &pyValue))
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto value = convertToValue(pyValue).asValues();
    _controller->setObjectsOfType(string(strType), string(strAttr), value);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyMethodDef PythonEmbedded::SplashMethods[] = {
    {
        (char*)"get_object_list", 
        PythonEmbedded::pythonGetObjectList, 
        METH_VARARGS, 
        (char*)"splash.get_object_list()\n"
               "\n"
               "Returns the list of objects from Splash"
    },
    {
        (char*)"get_object_types", 
        PythonEmbedded::pythonGetObjectTypes, 
        METH_VARARGS, 
        (char*)"splash.get_object_types()\n"
               "\n"
               "Retusn a dict of the objects types"
    },
    {
        (char*)"get_object_attribute_description", 
        PythonEmbedded::pythonGetObjectAttributeDescription, 
        METH_VARARGS, 
        (char*)"splash.get_object_attribute_description(objName, attribute)\n"
               "\n"
               "Returns the description for the attribute of the given object"
    },
    {
        (char*)"get_object_attribute",
        PythonEmbedded::pythonGetObjectAttribute,
        METH_VARARGS,
        (char*)"splash.get_object_attribute(objName, attribute)\n"
               "\n"
               "Returns the attribute value for the given object"
    },
    {
        (char*)"get_object_attributes",
        PythonEmbedded::pythonGetObjectAttributes,
        METH_VARARGS,
        (char*)"splash.get_object_attributes(objName)\n"
               "\n"
               "Returns a list of the available attributes for the given object"
    },
    {
        (char*)"get_object_links",
        PythonEmbedded::pythonGetObjectLinks,
        METH_VARARGS,
        (char*)"splash.get_object_links()\n"
               "\n"
               "Returns a dict of the links between all objects"
    },
    {
        (char*)"set_global",
        PythonEmbedded::pythonSetGlobal,
        METH_VARARGS,
        (char*)"splash.set_global(attribute, value)\n"
               "\n"
               "Set the given configuration-related attribute\n"
               "These attributes are listed in the \"world\" object type\n"
               "Returns True if the command was accepted"
    },
    {
        (char*)"set_object",
        PythonEmbedded::pythonSetObject,
        METH_VARARGS,
        (char*)"splash.set_object(objName, attribute, value)\n"
               "\n"
               "Set the attribute for the object to the given value\n"
               "Returns True if the command was accepted"
    },
    {
        (char*)"set_objects_of_type",
        PythonEmbedded::pythonSetObjectsOfType,
        METH_VARARGS,
        (char*)"splash.set_objects_of_type(objType, value)\n"
               "\n"
               "Set the attribute for all the objects of the given type\n"
               "Returns True if the command was accepted"
    },
    {nullptr, nullptr, 0, nullptr}
};

/*************/
PyModuleDef PythonEmbedded::SplashModule = {
    PyModuleDef_HEAD_INIT, "splash", nullptr, -1, PythonEmbedded::SplashMethods,
    nullptr, nullptr, nullptr, nullptr
};

/*************/
PyObject* PythonEmbedded::pythonInitSplash()
{
    if (!PythonEmbedded::_controller)
        return nullptr;

    PyObject* module {nullptr};

    module = PyModule_Create(&PythonEmbedded::SplashModule);
    if (!module)
        return nullptr;

    return module;
}

/*************/
// PythonEmbedded class definition
PythonEmbedded::PythonEmbedded(weak_ptr<RootObject> root)
    : ControllerObject(root)
{
    using namespace std::placeholders;

    _type = "python";
    registerAttributes();

    // Initialize the Splash python module
    if (_pythonInstances.fetch_add(1) == 0)
    {
        lock_guard<mutex> pythonLock(_pythonMutex);
        _controller = unique_ptr<ControllerObject>(new ControllerObject(_root));

        PyImport_AppendInittab("splash", &pythonInitSplash);
        Py_Initialize();
        PyEval_InitThreads();

        _pythonGlobalThreadState = PyThreadState_Get();
        PyEval_ReleaseThread(_pythonGlobalThreadState);
    }
}

/*************/
PythonEmbedded::~PythonEmbedded()
{
    stop();
    if (_pythonInstances.fetch_sub(1) == 1)
    {
        PyEval_AcquireThread(_pythonGlobalThreadState);
        Py_Finalize();
    }
}

/*************/
bool PythonEmbedded::setScriptFile(const string& src)
{
    _scriptName = Utils::getFilenameFromFilePath(src);
    _filepath = Utils::getPathFromFilePath(src, _configFilePath);

    if (!ifstream(_filepath + _scriptName, ios::in | ios::binary))
    {
        Log::get() << Log::WARNING << "PythonEmbedded::" << __FUNCTION__ << " - Could not load script file " << src << Log::endl;
        return false;
    }

    Log::get() << Log::MESSAGE << "PythonEmbedded::" << __FUNCTION__ << " - Loaded script file " << src << Log::endl;

    return run();
}

/*************/
bool PythonEmbedded::run()
{
    if (_filepath == "")
        return false;

    _loopThread = thread([&]() {
        _doLoop = true;
        loop();
    });

    return true;
}

/*************/
void PythonEmbedded::stop()
{
    // Stop and wait for the loop
    _doLoop = false;
    if (_loopThread.joinable())
        _loopThread.join();
}

/*************/
void PythonEmbedded::loop()
{
    PyObject *pName, *pModule, *pDict;
    PyObject *pArgs, *pValue;

    // Create the sub-interpreter
    PyEval_AcquireThread(_pythonGlobalThreadState);
    auto pyThreadState = Py_NewInterpreter();
    PyThreadState_Swap(pyThreadState);

    PyRun_SimpleString("import sys");
    // Load the module by its filename
    PyRun_SimpleString(("sys.path.append(\"" + _filepath + "\")").c_str());

    string moduleName = _scriptName.substr(0, _scriptName.rfind("."));
    pName = PyUnicode_FromString(moduleName.c_str());

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule)
    {
        // Run the initialization function
        auto pFuncInit = getFuncFromModule(pModule, "splash_init");
        if (pFuncInit)
        {
            PyObject_CallObject(pFuncInit, nullptr);
            if (PyErr_Occurred())
                PyErr_Print();
            Py_XDECREF(pFuncInit);
        }

        auto pFuncLoop = getFuncFromModule(pModule, "splash_loop");
        auto timerName = "PythonEmbedded_" + _name;

        PyThreadState_Swap(_pythonGlobalThreadState);
        PyEval_ReleaseThread(_pythonGlobalThreadState);

        // Run the module loop
        while (pFuncLoop && _doLoop)
        {
            Timer::get() << timerName;

            PyEval_AcquireThread(_pythonGlobalThreadState);
            PyThreadState_Swap(pyThreadState);

            PyObject_CallObject(pFuncLoop, nullptr);
            if (PyErr_Occurred())
                PyErr_Print();

            PyThreadState_Swap(_pythonGlobalThreadState);
            PyEval_ReleaseThread(_pythonGlobalThreadState);

            Timer::get() >> _loopDurationMs * 1000 >> timerName;
        }

        PyEval_AcquireThread(_pythonGlobalThreadState);
        PyThreadState_Swap(pyThreadState);

        Py_XDECREF(pFuncLoop);

        // Run the stop function
        auto pFuncStop = getFuncFromModule(pModule, "splash_stop");
        if (pFuncStop)
        {
            PyObject_CallObject(pFuncStop, nullptr);
            if (PyErr_Occurred())
                PyErr_Print();
            Py_XDECREF(pFuncStop);
        }

        Py_DECREF(pModule);

        PyThreadState_Swap(_pythonGlobalThreadState);
        PyEval_ReleaseThread(_pythonGlobalThreadState);
    }
    else
    {
        if (PyErr_Occurred())
            PyErr_Print();
        Log::get() << Log::WARNING << "PythonEmbedded::" << __FUNCTION__ << " - Error while importing module " << _filepath + _scriptName << Log::endl;

        PyThreadState_Swap(_pythonGlobalThreadState);
        PyEval_ReleaseThread(_pythonGlobalThreadState);
    }

    // Clean the interpreter
    PyEval_AcquireThread(_pythonGlobalThreadState);
    PyThreadState_Swap(pyThreadState);

    Py_EndInterpreter(pyThreadState);

    PyThreadState_Swap(_pythonGlobalThreadState);
    PyEval_ReleaseThread(_pythonGlobalThreadState);
}

/*************/
PyObject* PythonEmbedded::getFuncFromModule(PyObject* module, const string& name)
{
    auto pFunc = PyObject_GetAttrString(module, name.c_str());

    if (!pFunc || !PyCallable_Check(pFunc))
    {
        if (PyErr_Occurred())
            PyErr_Print();
        Log::get() << Log::WARNING << "PythonEmbedded::" << __FUNCTION__ << " - Cannot find function " << name << Log::endl;
    }

    return pFunc;
}

/*************/
PyObject* PythonEmbedded::convertFromValue(const Value& value)
{
    function<PyObject*(const Value&)> parseValue;
    parseValue = [&](const Value& v) -> PyObject* {
        PyObject* pyValue = nullptr;
        if (v.getType() == Value::Type::i)
            pyValue = Py_BuildValue("i", v.asInt());
        else if (v.getType() == Value::Type::l)
            pyValue = Py_BuildValue("l", v.asLong());
        else if (v.getType() == Value::Type::f)
            pyValue = Py_BuildValue("f", v.asFloat());
        else if (v.getType() == Value::Type::s)
            pyValue = Py_BuildValue("s", v.asString().c_str());
        else if (v.getType() == Value::Type::v)
        {
            auto values = v.asValues();
            pyValue = PyList_New(values.size());
            for (int i = 0; i < values.size(); ++i)
                PyList_SetItem(pyValue, i, parseValue(values[i]));
        }

        return pyValue;
    };

    return parseValue(value);
}

/*************/
Value PythonEmbedded::convertToValue(PyObject* pyObject)
{
    function<Value(PyObject*)> parsePyObject;
    parsePyObject = [&](PyObject* obj) -> Value {
        Value value;
        if (PyList_Check(obj))
        {
            auto values = Values();
            auto length = PyList_Size(obj);
            for (auto i = 0; i < length; ++i)
                values.push_back(parsePyObject(PyList_GetItem(obj, i)));
            value = values;
        }
        else if (PyLong_Check(obj))
        {
            value = PyLong_AsLong(obj);
        }
        else if (PyFloat_Check(obj))
        {
            value = PyFloat_AsDouble(obj);
        }
        else
        {
            value = "";
            char* strPtr = PyUnicode_AsUTF8(obj);
            if (strPtr)
                value = string(strPtr);
        }

        return value;
    };

    return parsePyObject(pyObject);
}

/*************/
void PythonEmbedded::registerAttributes()
{
    addAttribute("file", [&](const Values& args) {
        return setScriptFile(args[0].asString());
    }, [&]() -> Values {
        return {_filepath + _scriptName};
    }, {'s'});
    setAttributeDescription("file", "Set the path to the source Python file");
}

} // end of namespace
