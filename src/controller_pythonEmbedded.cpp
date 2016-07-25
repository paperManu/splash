#include "./controller_pythonEmbedded.h"

#include <fstream>
#include <Python.h>

#include "./osUtils.h"

using namespace std;

namespace Splash {

/*************/
PythonEmbedded::PythonEmbedded(weak_ptr<RootObject> root)
    : ControllerObject(root)
{
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
