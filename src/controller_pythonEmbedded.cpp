#include "./controller_pythonEmbedded.h"

#include <fstream>
#include <functional>
#include <mutex>

#include "./osUtils.h"

using namespace std;

namespace Splash
{

/*************/
atomic_int PythonEmbedded::_pythonInstances{0};
PyThreadState* PythonEmbedded::_pythonGlobalThreadState{nullptr};
PyObject* PythonEmbedded::SplashError{nullptr};

/*************/
PythonEmbedded* PythonEmbedded::getSplashInstance(PyObject* module)
{
    return static_cast<PythonEmbedded*>(PyCapsule_Import("splash.splash", 0));
}

/*************/
PyDoc_STRVAR(pythonGetObjectList_doc__,
    "Get the list of objects from Splash\n"
    "\n"
    "splash.get_object_list()\n"
    "\n"
    "Returns:\n"
    "  The list of objects instances\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetObjectList(PyObject* self, PyObject* args)
{
    auto that = getSplashInstance(self);
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    auto objects = that->getObjectNames();
    PyObject* pythonObjectList = PyList_New(objects.size());
    for (int i = 0; i < objects.size(); ++i)
        PyList_SetItem(pythonObjectList, i, Py_BuildValue("s", objects[i].c_str()));

    return pythonObjectList;
}

/*************/
PyDoc_STRVAR(pythonGetObjectTypes_doc__,
    "Get all the objects types\n"
    "\n"
    "splash.get_object_types()\n"
    "\n"
    "Returns:\n"
    "  The list of the object types\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetObjectTypes(PyObject* self, PyObject* args)
{
    auto that = getSplashInstance(self);
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

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
PyDoc_STRVAR(pythonGetObjectAttributeDescription_doc__,
    "Get the description for the attribute of the given object\n"
    "\n"
    "Signature:\n"
    "  splash.get_object_attribute_description(objectname, attribute)\n"
    "\n"
    "Args:\n"
    "  objectname (string): name of the object\n"
    "  attribute (string): wanted attribute\n"
    "\n"
    "Returns:\n"
    "  The description of the attribute\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetObjectAttributeDescription(PyObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance(self);
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    char* strName;
    char* strAttr;
    static const char* kwlist[] = {"objectname", "attribute", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ss", const_cast<char**>(kwlist), &strName, &strAttr))
    {
        PyErr_SetString(SplashError, "Wrong argument type or number");
        return Py_BuildValue("s", "");
    }

    auto result = that->getObjectAttributeDescription(string(strName), string(strAttr));
    if (result.size() == 0)
    {
        PyErr_SetString(SplashError, "Wrong argument type or number");
        return Py_BuildValue("s", "");
    }

    auto description = result[0].as<string>();
    return Py_BuildValue("s", description.c_str());
}

/*************/
PyDoc_STRVAR(pythonGetObjectAttribute_doc__,
    "Get the attribute value for the given object\n"
    "\n"
    "Signature:\n"
    "  splash.get_object_attribute(objectname, attribute)\n"
    "\n"
    "Args:\n"
    "  objectname (string): name of the object\n"
    "  attribute (string): wanted attribute\n"
    "\n"
    "Returns:\n"
    "  The value of the attribute\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetObjectAttribute(PyObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance(self);
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    char* strName;
    char* strAttr;
    static const char* kwlist[] = {"objectname", "attribute", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ss", const_cast<char**>(kwlist), &strName, &strAttr))
    {
        PyErr_SetString(SplashError, "Wrong argument type or number");
        return PyList_New(0);
    }

    auto result = that->getObjectAttribute(string(strName), string(strAttr));
    auto pyResult = convertFromValue(result);

    return pyResult;
}

/*************/
PyDoc_STRVAR(pythonGetObjectAttributes_doc__,
    "Get attributes for the given object\n"
    "\n"
    "Signature:\n"
    "  splash.get_object_attributes(objectname)\n"
    "\n"
    "Args:\n"
    "  objectname (string): name of the object\n"
    "\n"
    "Returns:\n"
    "  A list of the attributes of the object\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetObjectAttributes(PyObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance(self);
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    char* strName;
    static const char* kwlist[] = {"objectname", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", const_cast<char**>(kwlist), &strName))
    {
        PyErr_SetString(SplashError, "Wrong argument type or number");
        return PyDict_New();
    }

    auto result = that->getObjectAttributes(string(strName));
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
PyDoc_STRVAR(pythonGetObjectLinks_doc__,
    "Get the links between all objects (parents to children)\n"
    "\n"
    "Signature:\n"
    "  splash.get_object_links()\n"
    "\n"
    "Returns:\n"
    "  A dict of the links between the object\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetObjectLinks(PyObject* self, PyObject* args)
{
    auto that = getSplashInstance(self);
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

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
PyDoc_STRVAR(pythonGetObjectReversedLinks_doc__,
    "Get the links between all objects (children to parents)\n"
    "\n"
    "Signature:\n"
    "  splash.get_object_reversed_links()\n"
    "\n"
    "Returns:\n"
    "  A dict of the links between the object\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetObjectReversedLinks(PyObject* self, PyObject* args)
{
    auto that = getSplashInstance(self);
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    auto objects = that->getObjectReversedLinks();
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
PyDoc_STRVAR(pythonSetGlobal_doc__,
    "Set the given configuration-related attribute\n"
    "These attributes are listed in the \"world\" object type\n"
    "\n"
    "Signature:\n"
    "  splash.set_global(attribute, value)\n"
    "\n"
    "Args:\n"
    "  attribute (string): global attribute to set\n"
    "  value (object): value to set the attribute to\n"
    "\n"
    "Returns:\n"
    "  True if all went well\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSetGlobal(PyObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance(self);
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        Py_INCREF(Py_False);
        return Py_False;
    }

    char* attrName;
    PyObject* pyValue;
    static const char* kwlist[] = {"attribute", "value", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sO", const_cast<char**>(kwlist), &attrName, &pyValue))
    {
        PyErr_SetString(SplashError, "Wrong argument type or number");
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto value = convertToValue(pyValue).as<Values>();
    that->setGlobal(string(attrName), value);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonSetObject_doc__,
    "Set the attribute for the object to the given value\n"
    "\n"
    "Signature:\n"
    "  splash.set_object(objectname, attribute, value)\n"
    "\n"
    "Args:\n"
    "  objectname (string): object name\n"
    "  attribute (string): attribute to set\n"
    "  value (object): value to set the attribute to\n"
    "\n"
    "Returns:\n"
    "  True if all went well\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSetObject(PyObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance(self);
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        Py_INCREF(Py_False);
        return Py_False;
    }

    char* strName;
    char* strAttr;
    PyObject* pyValue;
    static const char* kwlist[] = {"objectname", "attribute", "value", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ssO", const_cast<char**>(kwlist), &strName, &strAttr, &pyValue))
    {
        PyErr_SetString(SplashError, "Wrong argument type or number");
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto value = convertToValue(pyValue).as<Values>();
    that->setObject(string(strName), string(strAttr), value);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonSetObjectsOfType_doc__,
    "Set the attribute for all the objects of the given type\n"
    "\n"
    "Signature\n"
    "  splash.set_objects_of_type(objecttype, attribute, value)\n"
    "\n"
    "Args:\n"
    "  objecttype (string): object type\n"
    "  attribute (string): attribute to set\n"
    "  value (object): value to set the attribute to\n"
    "\n"
    "Returns:\n"
    "  True if all went well\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSetObjectsOfType(PyObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance(self);
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        Py_INCREF(Py_False);
        return Py_False;
    }

    char* strType;
    char* strAttr;
    PyObject* pyValue;
    static const char* kwlist[] = {"objecttype", "attribute", "value", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ssO", const_cast<char**>(kwlist), &strType, &strAttr, &pyValue))
    {
        PyErr_SetString(SplashError, "Wrong argument type or number");
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto value = convertToValue(pyValue).as<Values>();
    that->setObjectsOfType(string(strType), string(strAttr), value);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonAddCustomAttribute_doc__,
    "Add a custom attribute to the script\n"
    "\n"
    "Signature:\n"
    "  splash.add_custom_attribute(variable)\n"
    "\n"
    "Args:\n"
    "  variable (string): global variable name to map as attribute\n"
    "\n"
    "Returns:\n"
    "  True if all went well\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available, or parameters are wrong");

PyObject* PythonEmbedded::pythonAddCustomAttribute(PyObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance(self);
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        Py_INCREF(Py_False);
        return Py_False;
    }

    char* strAttr;
    static const char* kwlist[] = {"variable", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", const_cast<char**>(kwlist), &strAttr))
    {
        PyErr_SetString(SplashError, "Wrong argument type or number");
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto attributeName = string(strAttr);

    // Check whether the variable is global to the script
    auto moduleDict = PyModule_GetDict(that->_pythonModule);
    auto object = PyDict_GetItemString(moduleDict, attributeName.c_str());
    if (!object)
    {
        PyErr_SetString(SplashError, (string("Could not find object named ") + attributeName + " in the global scope").c_str());
        Py_INCREF(Py_False);
        return Py_False;
    }

    that->addAttribute(attributeName,
        [=](const Values& args) {
            PyEval_AcquireThread(that->_pythonGlobalThreadState);
            PyThreadState_Swap(that->_pythonLocalThreadState);

            auto moduleDict = PyModule_GetDict(that->_pythonModule);
            PyObject* object = nullptr;
            if (args.size() == 1)
                object = convertFromValue(args[0]);
            else
                object = convertFromValue(args);
            if (PyDict_SetItemString(moduleDict, attributeName.c_str(), object) == -1)
            {
                PyErr_SetString(SplashError, (string("Could not set object named ") + attributeName).c_str());
                Py_DECREF(object);
                return false;
            }

            Py_DECREF(object);

            PyThreadState_Swap(that->_pythonGlobalThreadState);
            PyEval_ReleaseThread(that->_pythonGlobalThreadState);

            return true;
        },
        [=]() -> Values {
            PyEval_AcquireThread(that->_pythonGlobalThreadState);
            PyThreadState_Swap(that->_pythonLocalThreadState);

            auto moduleDict = PyModule_GetDict(that->_pythonModule);
            auto object = PyDict_GetItemString(moduleDict, attributeName.c_str());
            auto value = convertToValue(object);
            if (!object)
            {
                PyErr_SetString(SplashError, (string("Could not find object named ") + attributeName).c_str());
                return {};
            }

            PyThreadState_Swap(that->_pythonGlobalThreadState);
            PyEval_ReleaseThread(that->_pythonGlobalThreadState);

            if (value.getType() == Value::Type::v)
                return value.as<Values>();
            else
                return {value};
        },
        {});

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyMethodDef PythonEmbedded::SplashMethods[] = {{(char*)"get_object_list", (PyCFunction)PythonEmbedded::pythonGetObjectList, METH_VARARGS, pythonGetObjectList_doc__},
    {(char*)"get_object_types", (PyCFunction)PythonEmbedded::pythonGetObjectTypes, METH_VARARGS, pythonGetObjectTypes_doc__},
    {(char*)"get_object_attribute_description",
        (PyCFunction)PythonEmbedded::pythonGetObjectAttributeDescription,
        METH_VARARGS | METH_KEYWORDS,
        pythonGetObjectAttributeDescription_doc__},
    {(char*)"get_object_attribute", (PyCFunction)PythonEmbedded::pythonGetObjectAttribute, METH_VARARGS | METH_KEYWORDS, pythonGetObjectAttribute_doc__},
    {(char*)"get_object_attributes", (PyCFunction)PythonEmbedded::pythonGetObjectAttributes, METH_VARARGS | METH_KEYWORDS, pythonGetObjectAttributes_doc__},
    {(char*)"get_object_links", (PyCFunction)PythonEmbedded::pythonGetObjectLinks, METH_VARARGS | METH_KEYWORDS, pythonGetObjectLinks_doc__},
    {(char*)"get_object_reversed_links", (PyCFunction)PythonEmbedded::pythonGetObjectReversedLinks, METH_VARARGS | METH_KEYWORDS, pythonGetObjectReversedLinks_doc__},
    {(char*)"set_global", (PyCFunction)PythonEmbedded::pythonSetGlobal, METH_VARARGS | METH_KEYWORDS, pythonSetGlobal_doc__},
    {(char*)"set_object", (PyCFunction)PythonEmbedded::pythonSetObject, METH_VARARGS | METH_KEYWORDS, pythonSetObject_doc__},
    {(char*)"set_objects_of_type", (PyCFunction)PythonEmbedded::pythonSetObjectsOfType, METH_VARARGS | METH_KEYWORDS, pythonSetObjectsOfType_doc__},
    {(char*)"add_custom_attribute", (PyCFunction)PythonEmbedded::pythonAddCustomAttribute, METH_VARARGS | METH_KEYWORDS, pythonAddCustomAttribute_doc__},
    {nullptr, nullptr, 0, nullptr}};

/*************/
PyModuleDef PythonEmbedded::SplashModule = {PyModuleDef_HEAD_INIT, "splash", nullptr, -1, PythonEmbedded::SplashMethods, nullptr, nullptr, nullptr, nullptr};

/*************/
PyObject* PythonEmbedded::pythonInitSplash()
{
    PyObject* module{nullptr};

    module = PyModule_Create(&PythonEmbedded::SplashModule);
    if (!module)
        return nullptr;

    SplashError = PyErr_NewException((char*)"splash.error", nullptr, nullptr);
    if (SplashError)
    {
        Py_INCREF(SplashError);
        PyModule_AddObject(module, "error", SplashError);
    }

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
    _pythonLocalThreadState = Py_NewInterpreter();
    PyThreadState_Swap(_pythonLocalThreadState);

    // Set the current instance in a capsule
    auto module = PyImport_ImportModule("splash");
    auto capsule = PyCapsule_New((void*)this, "splash.splash", nullptr);
    PyDict_SetItemString(PyModule_GetDict(module), "splash", capsule);
    Py_DECREF(capsule);

    PyRun_SimpleString("import sys");
    // Load the module by its filename
    PyRun_SimpleString(("sys.path.append(\"" + _filepath + "\")").c_str());

    string moduleName = _scriptName.substr(0, _scriptName.rfind("."));
    pName = PyUnicode_FromString(moduleName.c_str());

    _pythonModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (_pythonModule)
    {
        // Run the initialization function
        auto pFuncInit = getFuncFromModule(_pythonModule, "splash_init");
        if (pFuncInit)
        {
            PyObject_CallObject(pFuncInit, nullptr);
            if (PyErr_Occurred())
                PyErr_Print();
            Py_XDECREF(pFuncInit);
        }

        auto pFuncLoop = getFuncFromModule(_pythonModule, "splash_loop");
        auto timerName = "PythonEmbedded_" + _name;

        PyThreadState_Swap(_pythonGlobalThreadState);
        PyEval_ReleaseThread(_pythonGlobalThreadState);

        // Run the module loop
        while (pFuncLoop && _doLoop)
        {
            Timer::get() << timerName;

            PyEval_AcquireThread(_pythonGlobalThreadState);
            PyThreadState_Swap(_pythonLocalThreadState);

            PyObject_CallObject(pFuncLoop, nullptr);
            if (PyErr_Occurred())
                PyErr_Print();

            PyThreadState_Swap(_pythonGlobalThreadState);
            PyEval_ReleaseThread(_pythonGlobalThreadState);

            Timer::get() >> _loopDurationMs * 1000 >> timerName;
        }

        PyEval_AcquireThread(_pythonGlobalThreadState);
        PyThreadState_Swap(_pythonLocalThreadState);

        Py_XDECREF(pFuncLoop);

        // Run the stop function
        auto pFuncStop = getFuncFromModule(_pythonModule, "splash_stop");
        if (pFuncStop)
        {
            PyObject_CallObject(pFuncStop, nullptr);
            if (PyErr_Occurred())
                PyErr_Print();
            Py_XDECREF(pFuncStop);
        }

        Py_DECREF(_pythonModule);

        PyThreadState_Swap(_pythonGlobalThreadState);
        PyEval_ReleaseThread(_pythonGlobalThreadState);
    }
    else
    {
        if (PyErr_Occurred())
            PyErr_Print();
        Log::get() << Log::WARNING << "PythonEmbedded::" << __FUNCTION__ << " - Error while importing module " << _filepath + _scriptName << Log::endl;

        PyErr_SetString(SplashError, "Error while importing module");

        PyThreadState_Swap(_pythonGlobalThreadState);
        PyEval_ReleaseThread(_pythonGlobalThreadState);
    }

    // Clean the interpreter
    PyEval_AcquireThread(_pythonGlobalThreadState);
    PyThreadState_Swap(_pythonLocalThreadState);

    Py_EndInterpreter(_pythonLocalThreadState);

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
            pyValue = Py_BuildValue("i", v.as<long>());
        else if (v.getType() == Value::Type::f)
            pyValue = Py_BuildValue("f", v.as<float>());
        else if (v.getType() == Value::Type::s)
            pyValue = Py_BuildValue("s", v.as<string>().c_str());
        else if (v.getType() == Value::Type::v)
        {
            auto values = v.as<Values>();
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
            value = static_cast<int64_t>(PyLong_AsLong(obj));
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
    addAttribute("file", [&](const Values& args) { return setScriptFile(args[0].as<string>()); }, [&]() -> Values { return {_filepath + _scriptName}; }, {'s'});
    setAttributeDescription("file", "Set the path to the source Python file");
}

} // end of namespace
