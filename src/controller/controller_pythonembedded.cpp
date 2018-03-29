#include "./controller/controller_pythonembedded.h"

#include <fstream>
#include <functional>
#include <mutex>

#include "./utils/log.h"
#include "./utils/osutils.h"

#define SPLASH_PYTHON_MAX_TRIES 200

using namespace std;

namespace Splash
{

/*************/
atomic_int PythonEmbedded::_pythonInstances{0};
PyThreadState* PythonEmbedded::_pythonGlobalThreadState{nullptr};
PyObject* PythonEmbedded::SplashError{nullptr};
atomic_int PythonEmbedded::sinkIndex{1};

/***********************/
// Sink Python wrapper //
/***********************/
void PythonEmbedded::pythonSinkDealloc(pythonSinkObject* self)
{
    if (self->initialized)
    {
        auto that = getSplashInstance();
        if (that)
        {
            that->setInScene("deleteObject", {self->sinkName});
            that->setInScene("deleteObject", {self->filterName});
        }

        if (self->linked)
        {
            auto result = pythonSinkUnlink(self);
            Py_XDECREF(result);
        }

        Py_XDECREF(self->lastBuffer);
    }

    Py_TYPE(self)->tp_free((PyObject*)self);
}

/*************/
PyObject* PythonEmbedded::pythonSinkNew(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwds*/)
{
    pythonSinkObject* self;
    self = reinterpret_cast<pythonSinkObject*>(type->tp_alloc(type, 0));
    return (PyObject*)self;
}

/*************/
int PythonEmbedded::pythonSinkInit(pythonSinkObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that)
    {
        PyErr_SetString(SplashError, "Can not access the Python embedded interpreter. Something is very wrong here...");
        return -1;
    }

    auto root = that->_root;
    if (!root)
    {
        PyErr_SetString(SplashError, "Can not access the root object");
        return -1;
    }

    char* source = nullptr;
    int width = 512;
    int height = 512;
    static char* kwlist[] = {(char*)"source", (char*)"width", (char*)"height", nullptr};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sii", kwlist, &source, &width, &height))
        return -1;

    self->width = width;
    self->height = height;
    self->framerate = 30;

    auto index = that->sinkIndex.fetch_add(1);
    self->sinkName = that->getName() + "_pythonsink_" + to_string(index);
    that->setInScene("add", {"sink", self->sinkName, root->getName()});

    // Wait until the sink is created
    int triesLeft = SPLASH_PYTHON_MAX_TRIES;
    while (!self->sink && --triesLeft)
    {
        self->sink = dynamic_pointer_cast<Sink>(root->getObject(self->sinkName));
        this_thread::sleep_for(chrono::milliseconds(5));
    }

    if (triesLeft == 0)
    {
        PyErr_SetString(SplashError, "Error while creating the Splash Sink object");
        return -1;
    }

    if (source)
    {
        auto newArgs = Py_BuildValue("(s)", source);
        auto newKwds = PyDict_New();
        auto result = pythonSinkLink(self, newArgs, newKwds);
        Py_XDECREF(result);
        Py_XDECREF(newArgs);
        Py_XDECREF(newKwds);
    }

    self->initialized = true;

    return 0;
}

/*************/
PyDoc_STRVAR(pythonSinkLink_doc__,
    "Link the sink to the given object\n"
    "\n"
    "splash.link_to(object_name)\n"
    "\n"
    "Returns:\n"
    "  True if the connection was successful\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSinkLink(pythonSinkObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto root = that->_root;
    if (!root)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (!self->sink)
        self->sink = dynamic_pointer_cast<Sink>(root->getObject(self->sinkName));

    if (!self->sink)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (self->linked)
    {
        PyErr_Warn(PyExc_Warning, "The sink is already linked to an object");
        Py_INCREF(Py_False);
        return Py_False;
    }

    char* source = nullptr;
    static char* kwlist[] = {(char*)"source", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &source))
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (source)
    {
        self->sourceName = string(source);
    }
    else
    {
        PyErr_Warn(PyExc_Warning, "No source object specified");
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto objects = that->getObjectNames();
    auto objectIt = std::find(objects.begin(), objects.end(), self->sourceName);
    if (objectIt == objects.end())
    {
        PyErr_Warn(PyExc_Warning, "The specified source object does not exist");
        Py_INCREF(Py_False);
        return Py_False;
    }

    self->filterName = self->sinkName + "_filter_" + self->sourceName;

    // Filter is added locally, we don't need (nor want) it in any other Scene
    that->setInScene("add", {"filter", self->filterName, root->getName()});
    that->setInScene("link", {self->sourceName, self->filterName});
    that->setInScene("link", {self->filterName, self->sinkName});
    that->setObjectAttribute(self->sinkName, "framerate", {self->framerate});
    that->setObjectAttribute(self->filterName, "sizeOverride", {self->width, self->height});

    self->linked = true;

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonSinkUnlink_doc__,
    "Unlink the sink from the connected object, if any\n"
    "\n"
    "splash.unlink()\n"
    "\n"
    "Returns:\n"
    "  True if the sink was connected\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSinkUnlink(pythonSinkObject* self)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto root = that->_root;
    if (!root)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (!self->sink)
        self->sink = dynamic_pointer_cast<Sink>(root->getObject(self->sinkName));

    if (!self->sink)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (!self->linked)
    {
        PyErr_Warn(PyExc_Warning, "The sink is not linked to any object");
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (self->opened)
    {
        auto result = pythonSinkClose(self);
        if (result == Py_False)
            return result;
        Py_XDECREF(result);
    }

    that->setInScene("unlink", {self->sourceName, self->filterName});
    that->setInScene("unlink", {self->filterName, self->sinkName});
    that->setInScene("deleteObject", {self->filterName});

    // Wait for the filter to be truly deleted. We do not try to get a shared_ptr
    // of the object, because we want it to be deleted. So we get the object list
    auto objectList = that->getObjectNames();
    while (std::find(objectList.begin(), objectList.end(), self->filterName) != objectList.end())
    {
        this_thread::sleep_for(chrono::milliseconds(5));
        objectList = that->getObjectNames();
    }

    self->linked = false;

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonSinkGrab_doc__,
    "Grab the latest image from the sink\n"
    "\n"
    "splash.grab()\n"
    "\n"
    "Returns:\n"
    "  The grabbed image as a bytes object\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSinkGrab(pythonSinkObject* self)
{
    auto that = getSplashInstance();
    if (!that)
        return Py_BuildValue("");

    auto root = that->_root;
    if (!root)
        return Py_BuildValue("");

    if (!self->sink)
        return Py_BuildValue("");

    if (!self->opened)
        return Py_BuildValue("");

    // The Sink wrapper can be opened although its Splash counterpart has not
    // received the order yet. We wait for that
    Values opened({false});
    while (!opened[0].as<bool>())
    {
        self->sink->getAttribute("opened", opened);
        this_thread::sleep_for(chrono::milliseconds(5));
    }

    // Due to the asynchronicity of passing messages to Splash, the frame may still
    // be at a wrong resolution if set_size was called. We test for this case.
    PyObject* buffer = nullptr;
    int triesLeft = SPLASH_PYTHON_MAX_TRIES;
    while (triesLeft)
    {
        auto frame = self->sink->getBuffer();
        if (frame.size() != self->width * self->height * 4 /* RGBA*/)
        {
            --triesLeft;
            this_thread::sleep_for(chrono::milliseconds(5));
            // Keeping the ratio may also have had some effects
            if (self->keepRatio)
            {
                auto realSize = that->getObjectAttribute(self->filterName, "sizeOverride");
                self->width = realSize[0].as<int>();
                self->height = realSize[1].as<int>();
            }
        }
        else
        {
            buffer = PyByteArray_FromStringAndSize(reinterpret_cast<char*>(frame.data()), frame.size());
            break;
        }
    }

    if (!buffer)
        return Py_BuildValue("");

    PyObject* tmp = nullptr;
    if (self->lastBuffer != nullptr)
        tmp = self->lastBuffer;
    self->lastBuffer = buffer;
    if (tmp != nullptr)
        Py_XDECREF(tmp);

    Py_INCREF(self->lastBuffer);
    return self->lastBuffer;
}

/*************/
PyDoc_STRVAR(pythonSinkSetSize_doc__,
    "Set the size of the grabbed images. Resizes the input accordingly\n"
    "\n"
    "splash.set_size(width, height)\n"
    "\n"
    "Args:\n"
    "  width (int): Desired width\n"
    "  height (int): Desired height\n"
    "\n"
    "Returns:\n"
    "  True if the size has been set correctly\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSinkSetSize(pythonSinkObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    int width = 512;
    int height = 0;
    static char* kwlist[] = {(char*)"width", (char*)"height", nullptr};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i|i", kwlist, &width, &height))
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    self->width = width;
    self->height = height;
    that->setObjectAttribute(self->filterName, "sizeOverride", {self->width, self->height});

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonSinkGetSize_doc__,
    "Get the size of the grabbed images\n"
    "\n"
    "splash.get_size()\n"
    "\n"
    "Returns:\n"
    "  width, height\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSinkGetSize(pythonSinkObject* self)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    Values size = that->getObjectAttribute(self->filterName, "sizeOverride");
    if (size.size() == 2)
        return Py_BuildValue("ii", size[0].as<int>(), size[1].as<int>());
    else
        return Py_BuildValue("");
}

/*************/
PyDoc_STRVAR(pythonSinkSetFramerate_doc__,
    "Set the framerate at which the sink should read the image\n"
    "\n"
    "splash.set_framerate(framerate)\n"
    "\n"
    "Args:\n"
    "  framerate (int): Framerate\n"
    "\n"
    "Returns:\n"
    "  True if the framerate has been set correctly\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSinkSetFramerate(pythonSinkObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    int framerate = 30;
    static char* kwlist[] = {(char*)"framerate", nullptr};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &framerate))
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    self->framerate = framerate;
    that->setObjectAttribute(self->sinkName, "framerate", {self->framerate});

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonSinkSetKeepRatio_doc__,
    "Set whether to keep the input image ratio when resizing\n"
    "\n"
    "splash.keep_ratio(keep_ratio)\n"
    "\n"
    "Args:\n"
    "  keep_ratio (int): Set to True to keep the ratio\n"
    "\n"
    "Returns:\n"
    "  True if the option has been set correctly\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSinkKeepRatio(pythonSinkObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    int keepRatio = false;
    static char* kwlist[] = {(char*)"keep_ratio", nullptr};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist, &keepRatio))
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    self->keepRatio = keepRatio;
    that->setObjectAttribute(self->filterName, "keepRatio", {static_cast<int>(keepRatio)});

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonSinkOpen_doc__,
    "Open the sink, which will start reading the input image\n"
    "\n"
    "splash.open()\n"
    "\n"
    "Returns:\n"
    "  True if the sink has been successfully opened\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSinkOpen(pythonSinkObject* self)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (!self->linked)
    {
        PyErr_Warn(PyExc_Warning, "The sink is not linked to any object");
        Py_INCREF(Py_False);
        return Py_False;
    }

    that->setObjectAttribute(self->sinkName, "opened", {1});
    self->opened = true;

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonSinkClose_doc__,
    "Close the sink\n"
    "\n"
    "splash.close()\n"
    "\n"
    "Returns:\n"
    "  True if the sink has been closed successfully\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSinkClose(pythonSinkObject* self)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (!self->opened)
    {
        PyErr_Warn(PyExc_Warning, "The sink is not opened");
        Py_INCREF(Py_False);
        return Py_False;
    }

    that->setObjectAttribute(self->sinkName, "opened", {0});
    self->opened = false;

    PyObject* tmp = nullptr;
    if (self->lastBuffer != nullptr)
        tmp = self->lastBuffer;
    self->lastBuffer = nullptr;
    if (tmp != nullptr)
        Py_XDECREF(tmp);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonSinkGetCaps_doc__,
    "Get a caps describing the grabbed buffers\n"
    "\n"
    "splash.get_caps()\n"
    "\n"
    "Returns:\n"
    "  A string holding the caps\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonSinkGetCaps(pythonSinkObject* self)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_BuildValue("s", "");
    }

    auto root = that->_root;
    if (!root)
        return Py_BuildValue("s", "");

    if (!self->sink)
        return Py_BuildValue("s", "");

    auto caps = self->sink->getCaps();
    return Py_BuildValue("s", caps.c_str());
}

// clang-format off
/*************/
PyMethodDef PythonEmbedded::SinkMethods[] = {
    {(const char*)"grab", (PyCFunction)PythonEmbedded::pythonSinkGrab, METH_NOARGS, pythonSinkGrab_doc__},
    {(const char*)"set_size", (PyCFunction)PythonEmbedded::pythonSinkSetSize, METH_VARARGS | METH_KEYWORDS, pythonSinkSetSize_doc__},
    {(const char*)"get_size", (PyCFunction)PythonEmbedded::pythonSinkGetSize, METH_VARARGS | METH_KEYWORDS, pythonSinkGetSize_doc__},
    {(const char*)"set_framerate", (PyCFunction)PythonEmbedded::pythonSinkSetFramerate, METH_VARARGS | METH_KEYWORDS, pythonSinkSetFramerate_doc__},
    {(const char*)"keep_ratio", (PyCFunction)PythonEmbedded::pythonSinkKeepRatio, METH_VARARGS | METH_KEYWORDS, pythonSinkSetKeepRatio_doc__},
    {(const char*)"open", (PyCFunction)PythonEmbedded::pythonSinkOpen, METH_NOARGS, pythonSinkOpen_doc__},
    {(const char*)"close", (PyCFunction)PythonEmbedded::pythonSinkClose, METH_NOARGS, pythonSinkClose_doc__},
    {(const char*)"get_caps", (PyCFunction)PythonEmbedded::pythonSinkGetCaps, METH_VARARGS | METH_KEYWORDS, pythonSinkGetCaps_doc__},
    {(const char*)"link_to", (PyCFunction)PythonEmbedded::pythonSinkLink, METH_VARARGS | METH_KEYWORDS, pythonSinkLink_doc__},
    {(const char*)"unlink", (PyCFunction)PythonEmbedded::pythonSinkUnlink, METH_NOARGS, pythonSinkUnlink_doc__},
    {nullptr}
};

/*************/
PyTypeObject PythonEmbedded::pythonSinkType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    (const char*) "splash.Sink",                               /* tp_name */
    sizeof(pythonSinkObject),                            /* tp_basicsize */
    0,                                                   /* tp_itemsize */
    (destructor)PythonEmbedded::pythonSinkDealloc,       /* tp_dealloc */
    0,                                                   /* tp_print */
    0,                                                   /* tp_getattr */
    0,                                                   /* tp_setattr */
    0,                                                   /* tp_reserved */
    0,                                                   /* tp_repr */
    0,                                                   /* tp_as_number */
    0,                                                   /* tp_as_sequence */
    0,                                                   /* tp_as_mapping */
    0,                                                   /* tp_hash  */
    0,                                                   /* tp_call */
    0,                                                   /* tp_str */
    0,                                                   /* tp_getattro */
    0,                                                   /* tp_setattro */
    0,                                                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,            /* tp_flags */
    (const char*)"Splash Sink Object",                         /* tp_doc */
    0,                                                   /* tp_traverse */
    0,                                                   /* tp_clear */
    0,                                                   /* tp_richcompare */
    0,                                                   /* tp_weaklistoffset */
    0,                                                   /* tp_iter */
    0,                                                   /* tp_iternext */
    PythonEmbedded::SinkMethods,                         /* tp_methods */
    0,                                                   /* tp_members */
    0,                                                   /* tp_getset */
    0,                                                   /* tp_base */
    0,                                                   /* tp_dict */
    0,                                                   /* tp_descr_get */
    0,                                                   /* tp_descr_set */
    0,                                                   /* tp_dictoffset */
    (initproc)PythonEmbedded::pythonSinkInit,            /* tp_init */
    0,                                                   /* tp_alloc */
    PythonEmbedded::pythonSinkNew                        /* tp_new */
};
// clang-format on

/*******************/
// Embedded Python //
/*******************/
PythonEmbedded* PythonEmbedded::getSplashInstance()
{
    auto that = static_cast<PythonEmbedded*>(PyCapsule_Import("splash._splash", 0));
    if (!that->_pythonModule)
    {
        // If _pythonModule is not set, it is most certainly because the Splash Python method
        // is called during the module load.
        PyErr_SetString(SplashError, "Splash method called in the global scope");
        return nullptr;
    }
    else
    {
        return that;
    }
}

/*************/
PyDoc_STRVAR(pythonGetInterpreterName_doc__,
    "Get the name of the Python interpreter running the script\n"
    "\n"
    "splash.get_interpreter_name()\n"
    "\n"
    "Returns:\n"
    "  The name of the interpreter\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetInterpreterName(PyObject* /*self*/, PyObject* /*args*/)
{
    auto that = getSplashInstance();
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    return Py_BuildValue("s", that->_name.c_str());
}

/*************/
PyDoc_STRVAR(pythonGetLogs_doc__,
    "Get the logs from Splash\n"
    "\n"
    "splash.get_logs()\n"
    "\n"
    "Returns:\n"
    "  The logs as a list\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetLogs(PyObject* /*self*/, PyObject* /*args*/)
{
    auto logs = Log::get().getLogs(Log::ERROR, Log::WARNING, Log::MESSAGE);
    PyObject* pythonLogList = PyList_New(logs.size());
    for (uint32_t i = 0; i < logs.size(); ++i)
        PyList_SetItem(pythonLogList, i, Py_BuildValue("s", logs[i].c_str()));

    return pythonLogList;
}

/*************/
PyDoc_STRVAR(pythonGetTimings_doc__,
    "Get the timings from Splash\n"
    "\n"
    "splash.get_timings()\n"
    "\n"
    "Returns:\n"
    "  The timers as a dict\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetTimings(PyObject* /*self*/, PyObject* /*args*/)
{
    auto& timings = Timer::get().getDurationMap();
    PyObject* pythonTimerDict = PyDict_New();
    for (auto& t : timings)
    {
        PyObject* val = Py_BuildValue("K", static_cast<uint64_t>(t.second));
        PyDict_SetItemString(pythonTimerDict, t.first.c_str(), val);
        Py_DECREF(val);
    }

    return pythonTimerDict;
}

/*************/
PyDoc_STRVAR(pythonGetMasterClock_doc__,
    "Get the master clock from Splash, in milliseconds\n"
    "\n"
    "splash.get_master_clock()\n"
    "\n"
    "Returns:\n"
    "  A tuple containing whether the clock is set, a boolean indicating whether it is paused, and the clock value in ms.\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetMasterClock(PyObject* /*self*/, PyObject* /*args*/)
{
    bool paused = false;
    int64_t clockMs = 0;
    bool isClockSet = Timer::get().getMasterClock<chrono::milliseconds>(clockMs, paused);
    PyObject* pyTuple = PyTuple_New(3);

    if (isClockSet)
    {
        Py_INCREF(Py_True);
        PyTuple_SetItem(pyTuple, 0, Py_True);
    }
    else
    {
        Py_INCREF(Py_False);
        PyTuple_SetItem(pyTuple, 0, Py_False);
    }

    if (paused)
    {
        Py_INCREF(Py_True);
        PyTuple_SetItem(pyTuple, 1, Py_True);
    }
    else
    {
        Py_INCREF(Py_False);
        PyTuple_SetItem(pyTuple, 1, Py_False);
    }

    PyTuple_SetItem(pyTuple, 2, Py_BuildValue("L", clockMs));

    return pyTuple;
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

PyObject* PythonEmbedded::pythonGetObjectList(PyObject* /*self*/, PyObject* /*args*/)
{
    auto that = getSplashInstance();
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    auto objects = that->getObjectNames();
    PyObject* pythonObjectList = PyList_New(objects.size());
    for (uint32_t i = 0; i < objects.size(); ++i)
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

PyObject* PythonEmbedded::pythonGetObjectTypes(PyObject* /*self*/, PyObject* /*args*/)
{
    auto that = getSplashInstance();
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
PyDoc_STRVAR(pythonGetObjectDescription_doc__,
    "Get the description of the given object\n"
    "\n"
    "Signature:\n"
    "  splash.get_object_description(objecttype)\n"
    "\n"
    "Args:\n"
    "  objecttype (string): type of the object\n"
    "\n"
    "Returns:\n"
    "  The short and long descriptions of the object type\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetObjectDescription(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    char* strType;
    static const char* kwlist[] = {"objecttype", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", const_cast<char**>(kwlist), &strType))
    {
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        return Py_BuildValue("ss", "", "");
    }

    auto description = that->getDescription(string(strType));
    auto shortDescription = that->getShortDescription(string(strType));
    return Py_BuildValue("ss", description.c_str(), shortDescription.c_str());
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

PyObject* PythonEmbedded::pythonGetObjectAttributeDescription(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
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
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        return Py_BuildValue("s", "");
    }

    auto result = that->getObjectAttributeDescription(string(strName), string(strAttr));
    if (result.size() == 0)
    {
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        return Py_BuildValue("s", "");
    }

    auto description = result[0].as<string>();
    return Py_BuildValue("s", description.c_str());
}

/*************/
PyDoc_STRVAR(pythonGetObjectType_doc__,
    "Get the type of the given object\n"
    "\n"
    "Signature:\n"
    "  splash.get_object_type(objectname)\n"
    "\n"
    "Args:\n"
    "  objectname (string): name of the object\n"
    "\n"
    "Returns:\n"
    "  The type of the object\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetObjectType(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    char* strName;
    static const char* kwlist[] = {"objectname", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", const_cast<char**>(kwlist), &strName))
    {
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        return PyList_New(0);
    }

    return convertFromValue(that->getObject(string(strName))->getType());
}

/*************/
PyDoc_STRVAR(pythonGetObjectsOfType_doc__,
    "Get the name of all the objects of the given type\n"
    "\n"
    "Signature:\n"
    "  splash.get_objects_of_type(objecttype)\n"
    "\n"
    "Args:\n"
    "  objecttype (string): type of the objects we want to get\n"
    "\n"
    "Returns:\n"
    "  The objects of the given type\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetObjectsOfType(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    char* strType;
    static const char* kwlist[] = {"objecttype", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", const_cast<char**>(kwlist), &strType))
    {
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        return PyList_New(0);
    }

    auto objects = that->getObjectsOfType(strType);
    PyObject* pythonObjectList = PyList_New(objects.size());

    int i = 0;
    for (auto& obj : objects)
    {
        PyList_SetItem(pythonObjectList, i, Py_BuildValue("s", obj->getName().c_str()));
        ++i;
    }

    return pythonObjectList;
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
    "  as_dict (bool): if True, returns the result as a dict (if values are named)\n"
    "\n"
    "Returns:\n"
    "  The value of the attribute\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetObjectAttribute(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    char* strName;
    char* strAttr;
    int asDict = 0;
    static const char* kwlist[] = {"objectname", "attribute", "as_dict", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ss|p", const_cast<char**>(kwlist), &strName, &strAttr, &asDict))
    {
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        return PyList_New(0);
    }

    auto result = that->getObjectAttribute(string(strName), string(strAttr));

    return convertFromValue(result, static_cast<bool>(asDict));
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

PyObject* PythonEmbedded::pythonGetObjectAttributes(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    char* strName;
    static const char* kwlist[] = {"objectname", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", const_cast<char**>(kwlist), &strName))
    {
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
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

PyObject* PythonEmbedded::pythonGetObjectLinks(PyObject* /*self*/, PyObject* /*args*/)
{
    auto that = getSplashInstance();
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
        for (uint32_t i = 0; i < obj.second.size(); ++i)
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

PyObject* PythonEmbedded::pythonGetObjectReversedLinks(PyObject* /*self*/, PyObject* /*args*/)
{
    auto that = getSplashInstance();
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
        for (uint32_t i = 0; i < obj.second.size(); ++i)
            PyList_SetItem(links, i, Py_BuildValue("s", obj.second[i].c_str()));
        PyDict_SetItemString(pythonObjectDict, obj.first.c_str(), links);
        Py_DECREF(links);
    }

    return pythonObjectDict;
}

/*************/
PyDoc_STRVAR(pythonGetTypesFromCategory_doc__,
    "Get the object types of the given category\n"
    "\n"
    "Signature:\n"
    "  splash.get_types_from_category(category)\n"
    "\n"
    "Args:\n"
    "  category (string): the desired category, can be misc, image or mesh currently\n"
    "\n"
    "Returns:\n"
    "  A list of the types\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available");

PyObject* PythonEmbedded::pythonGetTypesFromCategory(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return PyList_New(0);
    }

    char* strCategory;
    static const char* kwlist[] = {"category", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", const_cast<char**>(kwlist), &strCategory))
    {
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        return PyList_New(0);
    }

    auto cat = BaseObject::Category::MISC;
    if (strcmp(strCategory, "image") == 0)
        cat = BaseObject::Category::IMAGE;
    else if (strcmp(strCategory, "mesh") == 0)
        cat = BaseObject::Category::MESH;
    else if (strcmp(strCategory, "misc") == 0)
        cat = BaseObject::Category::MISC;
    else
        return PyList_New(0);

    auto types = that->getTypesFromCategory(cat);
    PyObject* pythonObjectList = PyList_New(types.size());
    for (uint32_t i = 0; i < types.size(); ++i)
        PyList_SetItem(pythonObjectList, i, Py_BuildValue("s", types[i].c_str()));

    return pythonObjectList;
}

/*************/
PyDoc_STRVAR(pythonSetGlobal_doc__,
    "Set the given configuration-related attribute\n"
    "These attributes are listed in the \"world\" object type\n"
    "\n"
    "Signature:\n"
    "  splash.set_world_attribute(attribute, value)\n"
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

PyObject* PythonEmbedded::pythonSetGlobal(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
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
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto value = convertToValue(pyValue).as<Values>();
    that->setWorldAttribute(string(attrName), value);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonSetObject_doc__,
    "Set the attribute for the object to the given value\n"
    "\n"
    "Signature:\n"
    "  splash.set_object_attribute(objectname, attribute, value)\n"
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

PyObject* PythonEmbedded::pythonSetObject(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
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
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto value = convertToValue(pyValue).as<Values>();
    that->setObjectAttribute(string(strName), string(strAttr), value);

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

PyObject* PythonEmbedded::pythonSetObjectsOfType(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
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
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
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

PyObject* PythonEmbedded::pythonAddCustomAttribute(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
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
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto attributeName = string(strAttr);

    // Check whether the variable is global to the script
    auto moduleDict = PyModule_GetDict(that->_pythonModule);
    auto object = PyDict_GetItemString(moduleDict, attributeName.c_str());
    if (!object)
    {
        PyErr_Warn(PyExc_Warning, (string("Could not find object named ") + attributeName + " in the global scope").c_str());
        Py_INCREF(Py_False);
        return Py_False;
    }

    // Get the value of the attribute if it has already been set during a previous setAttribute
    // (as attributes are not loaded in any order, the file may have been loaded after the other attributes are set)
    auto previousValue = that->getObjectAttribute(that->getName(), attributeName);

    // Add the attribute to the Python interpreter object
    // This will replace any previous (or default) attribute (setter and getter included)
    that->addAttribute(attributeName,
        [=](const Values& args) {
            bool calledFromPython = false;
            auto pyThreadDict = PyThreadState_GetDict();
            if (!pyThreadDict)
            {
                PyEval_AcquireThread(that->_pythonGlobalThreadState);
                PyThreadState_Swap(that->_pythonLocalThreadState);
                calledFromPython = true;
            }

            auto moduleDict = PyModule_GetDict(that->_pythonModule);
            PyObject* object = nullptr;
            if (args.size() == 1)
                object = convertFromValue(args[0]);
            else
                object = convertFromValue(args);
            if (PyDict_SetItemString(moduleDict, attributeName.c_str(), object) == -1)
            {
                PyErr_Warn(PyExc_Warning, (string("Could not set object named ") + attributeName).c_str());
                Py_DECREF(object);
                return false;
            }

            Py_DECREF(object);

            if (calledFromPython)
            {
                PyThreadState_Swap(that->_pythonGlobalThreadState);
                PyEval_ReleaseThread(that->_pythonGlobalThreadState);
            }

            return true;
        },
        [=]() -> Values {
            bool calledFromPython = false;
            auto pyThreadDict = PyThreadState_GetDict();
            if (!pyThreadDict)
            {
                PyEval_AcquireThread(that->_pythonGlobalThreadState);
                PyThreadState_Swap(that->_pythonLocalThreadState);
                calledFromPython = true;
            }

            auto moduleDict = PyModule_GetDict(that->_pythonModule);
            auto object = PyDict_GetItemString(moduleDict, attributeName.c_str());
            auto value = convertToValue(object);
            if (!object)
            {
                PyErr_Warn(PyExc_Warning, (string("Could not find object named ") + attributeName).c_str());
                return {};
            }

            if (calledFromPython)
            {
                PyThreadState_Swap(that->_pythonGlobalThreadState);
                PyEval_ReleaseThread(that->_pythonGlobalThreadState);
            }

            if (value.getType() == Value::Type::v)
                return value.as<Values>();
            else
                return {value};
        },
        {});

    // Set the previous attribute if needed
    if (!previousValue.empty())
        that->setObjectAttribute(that->getName(), attributeName, previousValue);

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyDoc_STRVAR(pythonRegisterAttributeCallback_doc__,
    "Register a callback to the attribute of the given Splash object\n"
    "\n"
    "Signature:\n"
    "  splash.register_attribute_callback(object_name, attribute, callback)\n"
    "\n"
    "Args:\n"
    "  object (string): Splash object name\n"
    "  attribute (string): Object attribute name\n"
    "  callback (callable): Callable to be used as a callback\n"
    "\n"
    "Returns:\n"
    "  The ID of the registered callback, 0 if it failed\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available, or parameters are wrong");

PyObject* PythonEmbedded::pythonRegisterAttributeCallback(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        return Py_BuildValue("I", 0);
    }

    char* objectName{nullptr};
    char* attributeName{nullptr};
    PyObject* callable{nullptr};
    static const char* kwlist[] = {"object", "attribute", "callable", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ssO", const_cast<char**>(kwlist), &objectName, &attributeName, &callable))
    {
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        return Py_BuildValue("I", 0);
    }

    if (!PyCallable_Check(callable))
    {
        PyErr_Warn(PyExc_Warning, "The object provided is not callable");
        return Py_BuildValue("I", 0);
    }

    auto splashObject = that->getObject(objectName);
    if (!splashObject)
    {
        PyErr_Warn(PyExc_Warning, "There is no Splash object with the given name");
        return Py_BuildValue("I", 0);
    }

    auto callbackFunc = [that, callable](const string& obj, const string& attr) {
        lock_guard<mutex> lockCb(that->_attributeCallbackMutex);

        PyEval_AcquireThread(that->_pythonGlobalThreadState);
        PyThreadState_Swap(that->_pythonLocalThreadState);

        auto pyTuple = PyTuple_New(2);
        PyTuple_SetItem(pyTuple, 0, Py_BuildValue("s", obj.c_str()));
        PyTuple_SetItem(pyTuple, 1, Py_BuildValue("s", attr.c_str()));

        PyObject_CallObject(callable, pyTuple);
        if (PyErr_Occurred())
            PyErr_Print();

        Py_DECREF(pyTuple);

        PyThreadState_Swap(that->_pythonGlobalThreadState);
        PyEval_ReleaseThread(that->_pythonGlobalThreadState);
    };

    lock_guard<mutex> lockCb(that->_attributeCallbackMutex);
    auto handle = splashObject->registerCallback(attributeName, callbackFunc);
    auto pyHandleId = Py_BuildValue("I", handle.getId());
    that->_attributeCallbackHandles[handle.getId()] = std::move(handle);

    return pyHandleId;
}

/*************/
PyDoc_STRVAR(pythonUnregisterAttributeCallback_doc__,
    "Unregister an attribute callback given its ID\n"
    "\n"
    "Signature:\n"
    "  splash.unregister_attribute_callback(callback_id)\n"
    "\n"
    "Args:\n"
    "  callback_id (unsigned int): Callback ID\n"
    "\n"
    "Returns:\n"
    "  True if the callback has been unregistered, False otherwise\n"
    "\n"
    "Raises:\n"
    "  splash.error: if Splash instance is not available, or parameters are wrong");

PyObject* PythonEmbedded::pythonUnregisterAttributeCallback(PyObject* /*self*/, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that || !that->_doLoop)
    {
        PyErr_SetString(SplashError, "Error accessing Splash instance");
        Py_INCREF(Py_False);
        return Py_False;
    }

    uint32_t callbackId{0};
    static const char* kwlist[] = {"callback_id", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", const_cast<char**>(kwlist), &callbackId))
    {
        PyErr_Warn(PyExc_Warning, "Wrong argument type or number");
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (callbackId == 0)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    lock_guard<mutex> lockCb(that->_attributeCallbackMutex);
    auto callbackIt = that->_attributeCallbackHandles.find(callbackId);
    if (callbackIt == that->_attributeCallbackHandles.end())
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    that->_attributeCallbackHandles.erase(callbackIt);

    Py_INCREF(Py_True);
    return Py_True;
}

// clang-format off
/*************/
PyMethodDef PythonEmbedded::SplashMethods[] = {
    {(const char*)"get_interpreter_name", (PyCFunction)PythonEmbedded::pythonGetInterpreterName, METH_VARARGS, pythonGetInterpreterName_doc__},
    {(const char*)"get_object_list", (PyCFunction)PythonEmbedded::pythonGetObjectList, METH_VARARGS, pythonGetObjectList_doc__},
    {(const char*)"get_logs", (PyCFunction)PythonEmbedded::pythonGetLogs, METH_VARARGS, pythonGetLogs_doc__},
    {(const char*)"get_timings", (PyCFunction)PythonEmbedded::pythonGetTimings, METH_VARARGS, pythonGetTimings_doc__},
    {(const char*)"get_master_clock", (PyCFunction)PythonEmbedded::pythonGetMasterClock, METH_VARARGS, pythonGetMasterClock_doc__},
    {(const char*)"get_object_types", (PyCFunction)PythonEmbedded::pythonGetObjectTypes, METH_VARARGS, pythonGetObjectTypes_doc__},
    {(const char*)"get_object_description", (PyCFunction)PythonEmbedded::pythonGetObjectDescription, METH_VARARGS, pythonGetObjectDescription_doc__},
    {(const char*)"get_object_attribute_description", (PyCFunction)PythonEmbedded::pythonGetObjectAttributeDescription, METH_VARARGS | METH_KEYWORDS, pythonGetObjectAttributeDescription_doc__},
    {(const char*)"get_object_type", (PyCFunction)PythonEmbedded::pythonGetObjectType, METH_VARARGS | METH_KEYWORDS, pythonGetObjectType_doc__},
    {(const char*)"get_objects_of_type", (PyCFunction)PythonEmbedded::pythonGetObjectsOfType, METH_VARARGS | METH_KEYWORDS, pythonGetObjectsOfType_doc__},
    {(const char*)"get_object_attribute", (PyCFunction)PythonEmbedded::pythonGetObjectAttribute, METH_VARARGS | METH_KEYWORDS, pythonGetObjectAttribute_doc__},
    {(const char*)"get_object_attributes", (PyCFunction)PythonEmbedded::pythonGetObjectAttributes, METH_VARARGS | METH_KEYWORDS, pythonGetObjectAttributes_doc__},
    {(const char*)"get_object_links", (PyCFunction)PythonEmbedded::pythonGetObjectLinks, METH_VARARGS | METH_KEYWORDS, pythonGetObjectLinks_doc__},
    {(const char*)"get_object_reversed_links", (PyCFunction)PythonEmbedded::pythonGetObjectReversedLinks, METH_VARARGS | METH_KEYWORDS, pythonGetObjectReversedLinks_doc__},
    {(const char*)"get_types_from_category", (PyCFunction)PythonEmbedded::pythonGetTypesFromCategory, METH_VARARGS | METH_KEYWORDS, pythonGetTypesFromCategory_doc__},
    {(const char*)"set_world_attribute", (PyCFunction)PythonEmbedded::pythonSetGlobal, METH_VARARGS | METH_KEYWORDS, pythonSetGlobal_doc__},
    {(const char*)"set_object_attribute", (PyCFunction)PythonEmbedded::pythonSetObject, METH_VARARGS | METH_KEYWORDS, pythonSetObject_doc__},
    {(const char*)"set_objects_of_type", (PyCFunction)PythonEmbedded::pythonSetObjectsOfType, METH_VARARGS | METH_KEYWORDS, pythonSetObjectsOfType_doc__},
    {(const char*)"add_custom_attribute", (PyCFunction)PythonEmbedded::pythonAddCustomAttribute, METH_VARARGS | METH_KEYWORDS, pythonAddCustomAttribute_doc__},
    {(const char*)"register_attribute_callback", (PyCFunction)PythonEmbedded::pythonRegisterAttributeCallback, METH_VARARGS | METH_KEYWORDS, pythonRegisterAttributeCallback_doc__},
    {(const char*)"unregister_attribute_callback", (PyCFunction)PythonEmbedded::pythonUnregisterAttributeCallback, METH_VARARGS | METH_KEYWORDS, pythonUnregisterAttributeCallback_doc__},
    {nullptr, nullptr, 0, nullptr}
};

/*************/
PyModuleDef PythonEmbedded::SplashModule = {
    PyModuleDef_HEAD_INIT, 
    "splash", 
    "Splash internal module", 
    -1, 
    PythonEmbedded::SplashMethods, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr
};
// clang-format on

/*************/
PyObject* PythonEmbedded::pythonInitSplash()
{
    PyObject* module{nullptr};

    module = PyModule_Create(&PythonEmbedded::SplashModule);
    if (!module)
    {
        Log::get() << Log::WARNING << "PythonEmbedded::" << __FUNCTION__ << " - Unable to load Splash module" << Log::endl;
        return nullptr;
    }

    if (PyType_Ready(&PythonEmbedded::pythonSinkType) < 0)
    {
        Log::get() << Log::WARNING << "PythonEmbedded::" << __FUNCTION__ << " - Sink type is not ready" << Log::endl;
        return nullptr;
    }
    Py_INCREF(&PythonEmbedded::pythonSinkType);
    PyModule_AddObject(module, "Sink", (PyObject*)&PythonEmbedded::pythonSinkType);

    SplashError = PyErr_NewException((const char*)"splash.error", PyExc_Exception, nullptr);
    if (SplashError)
    {
        Py_INCREF(SplashError);
        PyModule_AddObject(module, "error", SplashError);
    }

    return module;
}

/*************/
// PythonEmbedded class definition
PythonEmbedded::PythonEmbedded(RootObject* root)
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
    _filepath = Utils::getPathFromFilePath(src, _root->getConfigurationPath());

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
    PyObject* pName;

    // Create the sub-interpreter
    PyEval_AcquireThread(_pythonGlobalThreadState);
    _pythonLocalThreadState = Py_NewInterpreter();
    PyThreadState_Swap(_pythonLocalThreadState);

    // Set the current instance in a capsule
    auto module = PyImport_ImportModule("splash");
    auto capsule = PyCapsule_New((void*)this, "splash._splash", nullptr);
    PyDict_SetItemString(PyModule_GetDict(module), "_splash", capsule);
    Py_DECREF(capsule);

    PyRun_SimpleString("import sys");
    // Load the module by its filename
    PyRun_SimpleString(("sys.path.append(\"" + _filepath + "\")").c_str());

    // Set the script arguments
    PyRun_SimpleString(("sys.argv = []"));
    for (auto& arg : _pythonArgs)
        PyRun_SimpleString(("sys.argv.append(\"" + arg.as<string>() + "\")").c_str());

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

            Timer::get() >> 1.f / static_cast<float>(_updateRate) * 1000.f >> timerName;
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
PyObject* PythonEmbedded::convertFromValue(const Value& value, bool toDict)
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
            if (toDict)
            {
                pyValue = PyDict_New();
                for (uint32_t i = 0; i < values.size(); ++i)
                    if (values[i].isNamed())
                        PyDict_SetItem(pyValue, Py_BuildValue("s", values[i].getName().c_str()), parseValue(values[i]));
            }
            else
            {
                pyValue = PyList_New(values.size());
                for (uint32_t i = 0; i < values.size(); ++i)
                    PyList_SetItem(pyValue, i, parseValue(values[i]));
            }
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
    ControllerObject::registerAttributes();

    addAttribute("args", [&](const Values& args) {
        _pythonArgs = args;
        return true;
    });

    addAttribute("file", [&](const Values& args) { return setScriptFile(args[0].as<string>()); }, [&]() -> Values { return {_filepath + _scriptName}; }, {'s'});
    setAttributeDescription("file", "Set the path to the source Python file");

    addAttribute("loopRate",
        [&](const Values& args) {
            _updateRate = max(1, args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {_updateRate}; },
        {'n'});
    setAttributeDescription("loopRate", "Set the rate at which the loop is called");
}

} // end of namespace
