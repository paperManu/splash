#include "./controller/python/python_sink.h"

#include "./controller/controller_pythonembedded.h"

#define SPLASH_PYTHON_MAX_TRIES 200

using namespace std;

namespace Splash
{

/*************/
atomic_int PythonSink::PythonSinkObject::sinkIndex{1};

/***********************/
// Sink Python wrapper //
/***********************/
void PythonSink::pythonSinkDealloc(PythonSinkObject* self)
{
    auto that = PythonEmbedded::getInstance();
    if (self->linked)
    {
        auto result = pythonSinkUnlink(self);
        Py_XDECREF(result);
    }

    if (that)
    {
        that->setInScene("deleteObject", {self->sinkName});
        that->setInScene("deleteObject", {self->filterName});
    }

    Py_XDECREF(self->lastBuffer);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

/*************/
PyObject* PythonSink::pythonSinkNew(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwds*/)
{
    PythonSinkObject* self;
    self = reinterpret_cast<PythonSinkObject*>(type->tp_alloc(type, 0));
    return (PyObject*)self;
}

/*************/
int PythonSink::pythonSinkInit(PythonSinkObject* self, PyObject* args, PyObject* kwds)
{
    auto that = PythonEmbedded::getInstance();
    if (!that)
    {
        PyErr_SetString(PythonEmbedded::SplashError, "Can not access the Python embedded interpreter. Something is very wrong here...");
        return -1;
    }

    auto root = that->getRoot();
    if (!root)
    {
        PyErr_SetString(PythonEmbedded::SplashError, "Can not access the root object");
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

    auto index = self->sinkIndex.fetch_add(1);
    self->sinkName = that->getName() + "_pythonsink_" + to_string(index);
    that->setInScene("addObject", {"sink", self->sinkName, root->getName()});

    // Wait until the sink is created
    int triesLeft = SPLASH_PYTHON_MAX_TRIES;
    while (!self->sink && --triesLeft)
    {
        self->sink = dynamic_pointer_cast<Sink>(root->getObject(self->sinkName));
        this_thread::sleep_for(chrono::milliseconds(5));
    }

    if (triesLeft == 0)
    {
        PyErr_SetString(PythonEmbedded::SplashError, "Error while creating the Splash Sink object");
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

PyObject* PythonSink::pythonSinkLink(PythonSinkObject* self, PyObject* args, PyObject* kwds)
{
    auto that = PythonEmbedded::getInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto root = that->getRoot();
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
    that->setInScene("addObject", {"filter", self->filterName, root->getName()});
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

PyObject* PythonSink::pythonSinkUnlink(PythonSinkObject* self)
{
    auto that = PythonEmbedded::getInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    auto root = that->getRoot();
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

PyObject* PythonSink::pythonSinkGrab(PythonSinkObject* self)
{
    auto that = PythonEmbedded::getInstance();
    if (!that)
        return Py_BuildValue("");

    auto root = that->getRoot();
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

PyObject* PythonSink::pythonSinkSetSize(PythonSinkObject* self, PyObject* args, PyObject* kwds)
{
    auto that = PythonEmbedded::getInstance();
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

PyObject* PythonSink::pythonSinkGetSize(PythonSinkObject* self)
{
    auto that = PythonEmbedded::getInstance();
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

PyObject* PythonSink::pythonSinkSetFramerate(PythonSinkObject* self, PyObject* args, PyObject* kwds)
{
    auto that = PythonEmbedded::getInstance();
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

PyObject* PythonSink::pythonSinkKeepRatio(PythonSinkObject* self, PyObject* args, PyObject* kwds)
{
    auto that = PythonEmbedded::getInstance();
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

PyObject* PythonSink::pythonSinkOpen(PythonSinkObject* self)
{
    auto that = PythonEmbedded::getInstance();
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

PyObject* PythonSink::pythonSinkClose(PythonSinkObject* self)
{
    auto that = PythonEmbedded::getInstance();
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

PyObject* PythonSink::pythonSinkGetCaps(PythonSinkObject* self)
{
    auto that = PythonEmbedded::getInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_BuildValue("s", "");
    }

    auto root = that->getRoot();
    if (!root)
        return Py_BuildValue("s", "");

    if (!self->sink)
        return Py_BuildValue("s", "");

    auto caps = self->sink->getCaps();
    return Py_BuildValue("s", caps.c_str());
}

// clang-format off
/*************/
PyMethodDef PythonSink::SinkMethods[] = {
    {(const char*)"grab", (PyCFunction)PythonSink::pythonSinkGrab, METH_NOARGS, pythonSinkGrab_doc__},
    {(const char*)"set_size", (PyCFunction)PythonSink::pythonSinkSetSize, METH_VARARGS | METH_KEYWORDS, pythonSinkSetSize_doc__},
    {(const char*)"get_size", (PyCFunction)PythonSink::pythonSinkGetSize, METH_VARARGS | METH_KEYWORDS, pythonSinkGetSize_doc__},
    {(const char*)"set_framerate", (PyCFunction)PythonSink::pythonSinkSetFramerate, METH_VARARGS | METH_KEYWORDS, pythonSinkSetFramerate_doc__},
    {(const char*)"keep_ratio", (PyCFunction)PythonSink::pythonSinkKeepRatio, METH_VARARGS | METH_KEYWORDS, pythonSinkSetKeepRatio_doc__},
    {(const char*)"open", (PyCFunction)PythonSink::pythonSinkOpen, METH_NOARGS, pythonSinkOpen_doc__},
    {(const char*)"close", (PyCFunction)PythonSink::pythonSinkClose, METH_NOARGS, pythonSinkClose_doc__},
    {(const char*)"get_caps", (PyCFunction)PythonSink::pythonSinkGetCaps, METH_VARARGS | METH_KEYWORDS, pythonSinkGetCaps_doc__},
    {(const char*)"link_to", (PyCFunction)PythonSink::pythonSinkLink, METH_VARARGS | METH_KEYWORDS, pythonSinkLink_doc__},
    {(const char*)"unlink", (PyCFunction)PythonSink::pythonSinkUnlink, METH_NOARGS, pythonSinkUnlink_doc__},
    {nullptr}
};

/*************/
PyTypeObject PythonSink::pythonSinkType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    (const char*) "splash.Sink",                         /* tp_name */
    sizeof(PythonSinkObject),                            /* tp_basicsize */
    0,                                                   /* tp_itemsize */
    (destructor)PythonSink::pythonSinkDealloc,           /* tp_dealloc */
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
    (const char*)"Splash Sink Object",                   /* tp_doc */
    0,                                                   /* tp_traverse */
    0,                                                   /* tp_clear */
    0,                                                   /* tp_richcompare */
    0,                                                   /* tp_weaklistoffset */
    0,                                                   /* tp_iter */
    0,                                                   /* tp_iternext */
    PythonSink::SinkMethods,                             /* tp_methods */
    0,                                                   /* tp_members */
    0,                                                   /* tp_getset */
    0,                                                   /* tp_base */
    0,                                                   /* tp_dict */
    0,                                                   /* tp_descr_get */
    0,                                                   /* tp_descr_set */
    0,                                                   /* tp_dictoffset */
    (initproc)PythonSink::pythonSinkInit,                /* tp_init */
    0,                                                   /* tp_alloc */
    PythonSink::pythonSinkNew                            /* tp_new */
};
// clang-format on

} // namespace Splash
