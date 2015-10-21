#include "queue.h"

#include "image.h"
#include "image_ffmpeg.h"
#include "image_shmdata.h"
#include "world.h"

using namespace std;

namespace Splash
{

/*************/
Queue::Queue(RootObjectWeakPtr root)
    : BaseObject(root)
{
    _type = "queue";
    _world = dynamic_pointer_cast<World>(root.lock());

    registerAttributes();
}

/*************/
Queue::~Queue()
{
}

/*************/
bool Queue::linkTo(shared_ptr<BaseObject> obj)
{
    return false;
}

/*************/
bool Queue::unlinkFrom(shared_ptr<BaseObject> obj)
{
    return true;
}

/*************/
void Queue::update()
{
    if (_playlist.size() == 0)
        return;

    if (_startTime < 0)
        _startTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
    _currentTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count() - _startTime;

    auto source = _playlist[_currentSourceIndex];

    if (source.stop < _currentTime)
    {
        _currentSourceIndex++;
        // Auto looping for now
        if (_currentSourceIndex >= _playlist.size())
        {
            _currentSourceIndex = 0;
            _startTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
            _currentTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count() - _startTime;
        }
        source = _playlist[_currentSourceIndex];

        if (_currentSource)
        {
            _root.lock()->unregisterObject(_currentSource->getName());
            _currentSource.reset();
        }
    }

    if (source.start < _currentTime && source.stop > _currentTime)
    {
        if (!_currentSource)
        {
            _currentSource = createSource(source.type);
            _currentSource->setAttribute("file", {source.filename});
            _world.lock()->registerObject(_currentSource);
            _world.lock()->sendMessage(_name, "source", {source.type});
        }
    }
}

/*************/
shared_ptr<BaseObject> Queue::createSource(string type)
{
    auto source = shared_ptr<BaseObject>();

    if (type == "image")
    {
        source = make_shared<Image>();
    }
    else if (type == "image_ffmpeg")
    {
        source = make_shared<Image_FFmpeg>();
    }
    else if (type == "image_shmdata")
    {
        source = make_shared<Image_Shmdata>();
    }
    else
    {
        return {};
    }

    source->setName(_name + "_source");
    return source;
}

/*************/
void Queue::registerAttributes()
{
    _attribFunctions["loop"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        _loop = (bool)args[0].asInt();
        return true;
    }, [&]() -> Values {
        return {_loop};
    });
    _attribFunctions["loop"].doUpdateDistant(true);

    _attribFunctions["playlist"] = AttributeFunctor([&](const Values& args) {
        _playlist.clear();

        for (auto& it : args)
        {
            auto src = it.asValues();

            if (src.size() >= 4) // We need at least type, name, start and stop for each input
            {
                Source source;
                source.type = src[0].asString();
                source.filename = src[1].asString();
                source.start = (int64_t)(src[2].asFloat() * 1e6);
                source.stop = (int64_t)(src[3].asFloat() * 1e6);
                for (auto idx = 4; idx < src.size(); ++idx)
                    source.args.push_back(src[idx]);

                if (source.start < source.stop)
                    _playlist.push_back(source);
            }
        }

        return true;
    }, [&]() -> Values {
        Values playlist;
        for (auto& src : _playlist)
        {
            Values source;
            source.push_back(src.type);
            source.push_back(src.filename);
            source.push_back((double)src.start / 1e6);
            source.push_back((double)src.stop / 1e6);
            for (auto& v : src.args)
                source.push_back(v);

            playlist.emplace_back(std::move(source));
        }

        return playlist;
    });
    _attribFunctions["playlist"].doUpdateDistant(true);

    _attribFunctions["useClock"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        _useClock = args[0].asInt();
        return true;
    }, [&]() -> Values {
        return {(int)_useClock};
    });
    _attribFunctions["useClock"].doUpdateDistant(true);
}

/*************/
/*************/

/*************/
QueueSurrogate::QueueSurrogate(RootObjectWeakPtr root)
    : Texture(root)
{
    _type = "queue";
    _filter = make_shared<Filter>(root);
    _root.lock()->registerObject(_filter);

    registerAttributes();
}

/*************/
QueueSurrogate::~QueueSurrogate()
{
    _root.lock()->unregisterObject(_filter->getName());
}

/*************/
void QueueSurrogate::bind()
{
    _filter->bind();
}

/*************/
void QueueSurrogate::unbind()
{
    _filter->unbind();
}

/*************/
unordered_map<string, Values> QueueSurrogate::getShaderUniforms() const
{
    return _filter->getShaderUniforms();
}

/*************/
oiio::ImageSpec QueueSurrogate::getSpec() const
{
    return _filter->getSpec();
}

/*************/
bool QueueSurrogate::linkTo(shared_ptr<BaseObject> obj)
{
    return false;
}

/*************/
bool QueueSurrogate::unlinkFrom(shared_ptr<BaseObject> obj)
{
    return false;
}

/*************/
void QueueSurrogate::update()
{
    unique_lock<mutex> lock(_taskMutex);
    for (auto& task : _taskQueue)
        task();
    _taskQueue.clear();
    lock.unlock();
}

/*************/
void QueueSurrogate::registerAttributes()
{
    /*
     * Create the object for the current source type
     * Args holds the object type (Image, Texture...)
     */
    _attribFunctions["source"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        unique_lock<mutex> lock(_taskMutex);
        _taskQueue.push_back([=]() {
            auto sourceName = _name + "_source";

            if (_source)
            {
                _filter->unlinkFrom(_source);
                _root.lock()->unregisterObject(_source->getName());
                _source.reset();
            }

            auto type = args[0].asString();
            auto object = shared_ptr<BaseObject>();

            if (type.find("image") != string::npos)
            {
                object = make_shared<Image>();
                object->setRemoteType(type);
            }
            // TODO: add Texture_Syphon type
            //else if (type.find("texture_syphon") != string::npos)
            //{
            //    object = make_shared<Texture_Syphon>();
            //}
            else
            {
                return;
            }

            object->setName(sourceName);
            _root.lock()->registerObject(object);
            _filter->linkTo(object);
        });

        return true;
    });
}

} // end of namespace
