#include "./controller_pythonEmbedded.h"

#include <functional>
#include <fstream>
#include <mutex>

#include "./osUtils.h"

using namespace std;

namespace Splash {

/*************/
// Definition of the splash python module
mutex pythonMutex {};
PythonEmbedded* that {nullptr};

/*************/
PythonEmbedded* PythonEmbedded::getSplashInstance(PyObject* module)
{
    if (!module || !PyModule_Check(module))
        return nullptr;

    auto moduleDict = PyModule_GetDict(module);
    auto key = PyUnicode_FromString("splash");
    auto capsule = PyDict_GetItem(moduleDict, key);

    Py_DECREF(key);
    Py_DECREF(moduleDict);

    if (!capsule)
        return nullptr;

    auto ptr = static_cast<PythonEmbedded*>(PyCapsule_GetPointer(capsule, "splash.splash"));
    Py_DECREF(capsule);

    return ptr;
}

/*************/
PyObject* PythonEmbedded::pythonGetObjectList(PyObject* self, PyObject* args)
{
    auto that = getSplashInstance(self);
    if (!that)
        return nullptr;

    auto objects = that->getObjectNames();
    PyObject* pythonObjectList = PyList_New(objects.size());
    for (int i = 0; i < objects.size(); ++i)
        PyList_SetItem(pythonObjectList, i, Py_BuildValue("s", objects[i].c_str()));
    return pythonObjectList;
}

/*************/
PyObject* PythonEmbedded::pythonGetObjectTypes(PyObject* self, PyObject* args)
{
    auto that = getSplashInstance(self);
    if (!that)
        return nullptr;

    auto objects = that->getObjectTypes();
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
    auto that = getSplashInstance(self);
    if (!that)
        return nullptr;

    char* strName;
    char* strAttr;
    if (!PyArg_ParseTuple(args, "ss", &strName, &strAttr))
        return nullptr;

    auto result = that->getObjectAttributeDescription(string(strName), string(strAttr));
    if (result.size() == 0)
        return nullptr;

    auto description = result[0].asString();
    return Py_BuildValue("s", description.c_str());
}

/*************/
PyObject* PythonEmbedded::pythonGetObjectAttribute(PyObject* self, PyObject* args)
{
    auto that = getSplashInstance(self);
    if (!that)
        return nullptr;

    char* strName;
    char* strAttr;
    if (!PyArg_ParseTuple(args, "ss", &strName, &strAttr))
        return nullptr;

    auto result = that->getObjectAttribute(string(strName), string(strAttr));
    auto pyResult = convertFromValue(result);

    return pyResult;
}

/*************/
PyObject* PythonEmbedded::pythonGetObjectLinks(PyObject* self, PyObject* args)
{
    auto that = getSplashInstance(self);
    if (!that)
        return nullptr;

    auto objects = that->getObjectLinks();
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
    auto that = getSplashInstance(self);
    if (!that)
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
    Py_XDECREF(pyValue);
    that->setGlobal(string(strName), value);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyObject* PythonEmbedded::pythonSetObject(PyObject* self, PyObject* args)
{
    auto that = getSplashInstance(self);
    if (!that)
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
    Py_XDECREF(pyValue);
    that->setObject(string(strName), string(strAttr), value);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyObject* PythonEmbedded::pythonSetObjectsOfType(PyObject* self, PyObject* args)
{
    auto that = getSplashInstance(self);
    if (!that)
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
    Py_XDECREF(pyValue);
    that->setObjectsOfType(string(strType), string(strAttr), value);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyMethodDef PythonEmbedded::SplashMethods[] = {
    {
        (char*)"get_object_list", 
        PythonEmbedded::pythonGetObjectList, 
        METH_VARARGS, 
        (char*)"Get the object list from Splash"
    },
    {
        (char*)"get_object_types", 
        PythonEmbedded::pythonGetObjectTypes, 
        METH_VARARGS, 
        (char*)"Get a dict of the objects types"
    },
    {
        (char*)"get_object_attribute_description", 
        PythonEmbedded::pythonGetObjectAttributeDescription, 
        METH_VARARGS, 
        (char*)"Get the description for the attribute of the given object"
    },
    {
        (char*)"get_object_attribute",
        PythonEmbedded::pythonGetObjectAttribute,
        METH_VARARGS,
        (char*)"Get the attribute value for the given object"
    },
    {
        (char*)"get_object_links",
        PythonEmbedded::pythonGetObjectLinks,
        METH_VARARGS,
        (char*)"Get the links between all objects"
    },
    {
        (char*)"set_global",
        PythonEmbedded::pythonSetGlobal,
        METH_VARARGS,
        (char*)"Set the given configuration-related attribute"
    },
    {
        (char*)"set_object",
        PythonEmbedded::pythonSetObject,
        METH_VARARGS,
        (char*)"Set the attribute for the object to the given value"
    },
    {
        (char*)"set_objects_of_type",
        PythonEmbedded::pythonSetObjectsOfType,
        METH_VARARGS,
        (char*)"Set the attribute for all the objects of the given type"
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
    if (!that)
        return nullptr;

    PyObject* module {nullptr};

    module = PyModule_Create(&PythonEmbedded::SplashModule);
    if (!module)
        return nullptr;

    PyObject* splashCapsule = PyCapsule_New((void*)that, "splash.splash", nullptr);
    if (!splashCapsule)
        return nullptr;

    PyModule_AddObject(module, "splash", splashCapsule);

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
}

/*************/
PythonEmbedded::~PythonEmbedded()
{
    if (_doLoop)
        stop();
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
bool PythonEmbedded::stop()
{
    auto loopFuture = _loopThreadPromise.get_future();
    _doLoop = false;
    loopFuture.wait();
    return loopFuture.get();
}

/*************/
void PythonEmbedded::loop()
{
    bool returnValue = true;

    PyObject *pName, *pModule, *pDict, *pFunc;
    PyObject *pArgs, *pValue;

    // Initialize the Splash python module
    {
        lock_guard<mutex> pythonLock(pythonMutex);
        that = this;
        PyImport_AppendInittab("splash", &pythonInitSplash);
    }

    // Load the module by its filename
    Py_Initialize();
    
    PyRun_SimpleString("import sys");
    PyRun_SimpleString(("sys.path.append(\"" + _filepath + "\")").c_str());

    string moduleName = _scriptName.substr(0, _scriptName.rfind("."));
    pName = PyUnicode_FromString(moduleName.c_str());

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule)
    {
        pFunc = PyObject_GetAttrString(pModule, moduleName.c_str());

        if (!pFunc || !PyCallable_Check(pFunc))
        {
            if (PyErr_Occurred())
                PyErr_Print();
            Log::get() << Log::WARNING << "PythonEmbedded::" << __FUNCTION__ << " - Cannot find function " << _scriptName << Log::endl;
            returnValue = false;
        }
        else
        {
            auto pReturn = PyObject_CallObject(pFunc, nullptr);
        }

        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
    else
    {
        if (PyErr_Occurred())
            PyErr_Print();
        Log::get() << Log::WARNING << "PythonEmbedded::" << __FUNCTION__ << " - Error while importing module " << _filepath + _scriptName << Log::endl;
        returnValue = false;
    }

    Py_Finalize();

    _loopThreadPromise.set_value(returnValue);
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
            char* strPtr;
            PyArg_ParseTuple(obj, "s", &strPtr);
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
        return {_filepath};
    }, {'s'});
    setAttributeDescription("file", "Set the path to the source Python file");
}

} // end of namespace
