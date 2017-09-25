#include "./controller_pythonEmbedded.h"

#include <fstream>
#include <functional>
#include <mutex>

#include "./log.h"
#include "./osUtils.h"

using namespace std;

namespace Splash
{

/*************/
atomic_int PythonEmbedded::_pythonInstances{0};
PyThreadState* PythonEmbedded::_pythonGlobalThreadState{nullptr};
PyObject* PythonEmbedded::SplashError{nullptr};

/***********************/
// Sink Python wrapper //
/***********************/
void PythonEmbedded::pythonSinkDealloc(pythonSinkObject* self)
{
    auto that = getSplashInstance();
    if (that)
    {
        that->setInScene("deleteObject", {self->sinkName});
        that->setInScene("deleteObject", {self->filterName});
    }

    Py_XDECREF(self->lastBuffer);

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

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|ii", kwlist, &source, &width, &height))
        return -1;

    if (source)
    {
        self->sourceName = string(source);
    }
    else
    {
        PyErr_SetString(SplashError, "No source object specified");
        return -1;
    }

    self->width = width;
    self->height = height;
    self->framerate = 30;

    auto objects = that->getObjectNames();
    auto objectIt = std::find(objects.begin(), objects.end(), self->sourceName);
    if (objectIt == objects.end())
    {
        PyErr_SetString(SplashError, "The specified source object does not exist");
        return -1;
    }

    self->sinkName = that->getName() + "_sink_" + self->sourceName;
    self->filterName = that->getName() + "_filter_" + self->sourceName;

    // Objects are added locally, we don't need (nor want) them in any other Scene
    that->setInScene("add", {"sink", self->sinkName, root->getName()});
    that->setInScene("add", {"filter", self->filterName, root->getName()});
    that->setInScene("link", {self->sourceName, self->filterName});
    that->setInScene("link", {self->filterName, self->sinkName});
    that->setObjectAttribute(self->sinkName, "framerate", {self->framerate});
    that->setObjectAttribute(self->filterName, "sizeOverride", {self->width, self->height});

    return 0;
}

/*************/
PyObject* PythonEmbedded::pythonSinkGrab(pythonSinkObject* self)
{
    auto that = getSplashInstance();
    if (!that)
        return Py_BuildValue("");

    auto root = that->_root;
    if (!root)
        return Py_BuildValue("");

    if (!self->sink)
        self->sink = dynamic_pointer_cast<Sink>(root->getObject(self->sinkName));

    if (!self->sink)
        return Py_BuildValue("");

    auto frame = self->sink->getBuffer();
    PyObject* buffer = PyByteArray_FromStringAndSize(reinterpret_cast<char*>(frame.data()), frame.size());

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
PyObject* PythonEmbedded::pythonSinkSetSize(pythonSinkObject* self, PyObject* args, PyObject* kwds)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    int width = 512;
    int height = 512;
    static char* kwlist[] = {(char*)"width", (char*)"height", nullptr};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ii", kwlist, &width, &height))
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
PyObject* PythonEmbedded::pythonSinkOpen(pythonSinkObject* self)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    that->setObjectAttribute(self->sinkName, "opened", {1});

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
PyObject* PythonEmbedded::pythonSinkClose(pythonSinkObject* self)
{
    auto that = getSplashInstance();
    if (!that)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    that->setObjectAttribute(self->sinkName, "opened", {0});

    Py_INCREF(Py_True);
    return Py_True;
}

/*************/
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
        self->sink = dynamic_pointer_cast<Sink>(root->getObject(self->sinkName));

    if (!self->sink)
        return Py_BuildValue("s", "");

    auto caps = self->sink->getCaps();
    return Py_BuildValue("s", caps.c_str());
}

// clang-format off
/*************/
PyMethodDef PythonEmbedded::SinkMethods[] = {
    {(const char*)"grab", (PyCFunction)PythonEmbedded::pythonSinkGrab, METH_VARARGS, (const char*)"Get a copy of the buffer in the sink"},
    {(const char*)"set_size", (PyCFunction)PythonEmbedded::pythonSinkSetSize, METH_VARARGS, (const char*)"Set the size of the grabbed image"},
    {(const char*)"set_framerate", (PyCFunction)PythonEmbedded::pythonSinkSetFramerate, METH_VARARGS, (const char*)"Set the framerate of the sink"},
    {(const char*)"open", (PyCFunction)PythonEmbedded::pythonSinkOpen, METH_VARARGS, (const char*)"Open the sink"},
    {(const char*)"close", (PyCFunction)PythonEmbedded::pythonSinkClose, METH_VARARGS, (const char*)"Close the sink"},
    {(const char*)"get_caps", (PyCFunction)PythonEmbedded::pythonSinkGetCaps, METH_VARARGS, (const char*)"Get the caps of the buffer"},
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

PyObject* PythonEmbedded::pythonGetLogs(PyObject* self, PyObject* args)
{
    auto logs = Log::get().getLogs(Log::ERROR, Log::WARNING, Log::MESSAGE);
    PyObject* pythonLogList = PyList_New(logs.size());
    for (int i = 0; i < logs.size(); ++i)
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

PyObject* PythonEmbedded::pythonGetTimings(PyObject* self, PyObject* args)
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
    auto that = getSplashInstance();
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

PyObject* PythonEmbedded::pythonGetObjectDescription(PyObject* self, PyObject* args, PyObject* kwds)
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
        PyErr_SetString(SplashError, "Wrong argument type or number");
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

PyObject* PythonEmbedded::pythonGetObjectAttributeDescription(PyObject* self, PyObject* args, PyObject* kwds)
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
        for (int i = 0; i < obj.second.size(); ++i)
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

PyObject* PythonEmbedded::pythonGetTypesFromCategory(PyObject* self, PyObject* args, PyObject* kwds)
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
        PyErr_SetString(SplashError, "Wrong argument type or number");
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
    for (int i = 0; i < types.size(); ++i)
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

PyObject* PythonEmbedded::pythonSetGlobal(PyObject* self, PyObject* args, PyObject* kwds)
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
        PyErr_SetString(SplashError, "Wrong argument type or number");
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

PyObject* PythonEmbedded::pythonSetObject(PyObject* self, PyObject* args, PyObject* kwds)
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
        PyErr_SetString(SplashError, "Wrong argument type or number");
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

PyObject* PythonEmbedded::pythonSetObjectsOfType(PyObject* self, PyObject* args, PyObject* kwds)
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

    // Get the value of the attribute if it has already been set during a previous setAttribute
    // (as attributes are not loaded in any order, the file may have been loaded after the other attributes are set)
    auto previousValue = that->getObjectAttribute(that->getName(), attributeName);

    // Add the attribute to the Python interpreter object
    // This will replace any previous (or default) attribute (setter and getter included)
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

    // Set the previous attribute if needed
    if (!previousValue.empty())
        that->setObjectAttribute(that->getName(), attributeName, previousValue);

    Py_INCREF(Py_True);
    return Py_True;
}

// clang-format off
/*************/
PyMethodDef PythonEmbedded::SplashMethods[] = {
    {(const char*)"get_object_list", (PyCFunction)PythonEmbedded::pythonGetObjectList, METH_VARARGS, pythonGetObjectList_doc__},
    {(const char*)"get_logs", (PyCFunction)PythonEmbedded::pythonGetLogs, METH_VARARGS, pythonGetLogs_doc__},
    {(const char*)"get_timings", (PyCFunction)PythonEmbedded::pythonGetTimings, METH_VARARGS, pythonGetTimings_doc__},
    {(const char*)"get_object_types", (PyCFunction)PythonEmbedded::pythonGetObjectTypes, METH_VARARGS, pythonGetObjectTypes_doc__},
    {(const char*)"get_object_description", (PyCFunction)PythonEmbedded::pythonGetObjectDescription, METH_VARARGS, pythonGetObjectDescription_doc__},
    {(const char*)"get_object_attribute_description",
        (PyCFunction)PythonEmbedded::pythonGetObjectAttributeDescription,
        METH_VARARGS | METH_KEYWORDS,
        pythonGetObjectAttributeDescription_doc__},
    {(const char*)"get_object_attribute", (PyCFunction)PythonEmbedded::pythonGetObjectAttribute, METH_VARARGS | METH_KEYWORDS, pythonGetObjectAttribute_doc__},
    {(const char*)"get_object_attributes", (PyCFunction)PythonEmbedded::pythonGetObjectAttributes, METH_VARARGS | METH_KEYWORDS, pythonGetObjectAttributes_doc__},
    {(const char*)"get_object_links", (PyCFunction)PythonEmbedded::pythonGetObjectLinks, METH_VARARGS | METH_KEYWORDS, pythonGetObjectLinks_doc__},
    {(const char*)"get_object_reversed_links", (PyCFunction)PythonEmbedded::pythonGetObjectReversedLinks, METH_VARARGS | METH_KEYWORDS, pythonGetObjectReversedLinks_doc__},
    {(const char*)"get_types_from_category", (PyCFunction)PythonEmbedded::pythonGetTypesFromCategory, METH_VARARGS | METH_KEYWORDS, pythonGetTypesFromCategory_doc__},
    {(const char*)"set_world_attribute", (PyCFunction)PythonEmbedded::pythonSetGlobal, METH_VARARGS | METH_KEYWORDS, pythonSetGlobal_doc__},
    {(const char*)"set_object_attribute", (PyCFunction)PythonEmbedded::pythonSetObject, METH_VARARGS | METH_KEYWORDS, pythonSetObject_doc__},
    {(const char*)"set_objects_of_type", (PyCFunction)PythonEmbedded::pythonSetObjectsOfType, METH_VARARGS | METH_KEYWORDS, pythonSetObjectsOfType_doc__},
    {(const char*)"add_custom_attribute", (PyCFunction)PythonEmbedded::pythonAddCustomAttribute, METH_VARARGS | METH_KEYWORDS, pythonAddCustomAttribute_doc__},
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

    SplashError = PyErr_NewException((const char*)"splash.error", nullptr, nullptr);
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
    PyObject *pName, *pModule, *pDict;
    PyObject *pArgs, *pValue;

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
    ControllerObject::registerAttributes();

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
