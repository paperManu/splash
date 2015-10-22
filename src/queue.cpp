#include "queue.h"

#include "image.h"
#if HAVE_FFMPEG
    #include "image_ffmpeg.h"
#endif
#if HAVE_SHMDATA
    #include "image_shmdata.h"
#endif
#include "log.h"
#include "timer.h"
#include "world.h"

#define DISTANT_NAME_SUFFIX "_source"

using namespace std;

namespace Splash
{

/*************/
Queue::Queue(RootObjectWeakPtr root)
    : BufferObject(root)
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
unique_ptr<SerializedObject> Queue::serialize() const
{
    if (_currentSource)
        return _currentSource->serialize();
    else
        return {};
}

/*************/
string Queue::getDistantName() const
{
    return _name + DISTANT_NAME_SUFFIX;
}

/*************/
void Queue::update()
{
    if (_playlist.size() == 0)
        return;

    if (_startTime < 0)
        _startTime = Timer::getTime<chrono::microseconds>();

    int64_t masterClockTime;
    if (_useClock && Timer::get().getMasterClock<chrono::microseconds>(masterClockTime))
    {
        _currentTime = masterClockTime;
    }
    else
    {
        auto previousTime = _currentTime;
        _currentTime = Timer::getTime<chrono::microseconds>() - _startTime;

        if (_paused)
        {
            _startTime = _startTime + (_currentTime - previousTime);
            _currentTime = previousTime;
            if (_currentSource)
                _currentSource->setAttribute("pause", {1});
            return;
        }
        else if (_currentSource)
            _currentSource->setAttribute("pause", {0});
    }

    auto source = _playlist[0];
    if (_playing)
        source = _playlist[_currentSourceIndex];

    // Get the current index regarding the current time
    uint32_t sourceIndex = 0;
    for (auto& playSource : _playlist)
    {
        if (playSource.start <= _currentTime && playSource.stop > _currentTime)
            break;
        sourceIndex++;
    }

    // If loop is activated, and master clock is not used
    if (!_useClock && _loop && sourceIndex >= _playlist.size())
    {
        sourceIndex = 0;
        _startTime = Timer::getTime<chrono::microseconds>();
        _currentTime = 0;
    }

    // If the index changed
    if (sourceIndex != _currentSourceIndex)
    {
        if (_playing)
        {
            Log::get() << Log::MESSAGE << "Queue::" << __FUNCTION__ << " - Finished playing file: " << source.filename << Log::endl;
            _playing = false;
        }

        _currentSourceIndex = sourceIndex;
        _currentSource.reset();
        
        if (sourceIndex >= _playlist.size())
        {
            _currentSource = make_shared<Image>();
            _world.lock()->sendMessage(_name, "source", {"image"});
        }
        else
        {
            source = _playlist[_currentSourceIndex];
            _currentSource = createSource(source.type);

            if (_currentSource)
                _playing = true;
            else
                _currentSource = make_shared<Image>();

            _currentSource->setAttribute("file", {source.filename});

            if (_useClock)
            {
                // If we use the master clock, set a timeshift to be correctly placed in the video
                // (as the source gets its clock from the same Timer)
                _currentSource->setAttribute("timeShift", {-(float)source.start / 1e6});
                _currentSource->setAttribute("useClock", {1});
            }

            _world.lock()->sendMessage(_name, "source", {source.type});

            Log::get() << Log::MESSAGE << "Queue::" << __FUNCTION__ << " - Playing file: " << source.filename << Log::endl;
        }
    }

    if (!_useClock && _seeked)
    {
        // If we don't use the master clock, we want to seek accordingly in the file
        _currentSource->setAttribute("seek", {(float)(_currentTime - source.start) / 1e6});
        _seeked = false;
    }
    
    if (_currentSource)
        _currentSource->update();
}

/*************/
shared_ptr<BufferObject> Queue::createSource(string type)
{
    auto source = shared_ptr<BufferObject>();

    if (type == "image")
    {
        source = make_shared<Image>();
    }
#if HAVE_SHMDATA
    else if (type == "image_ffmpeg")
    {
        source = make_shared<Image_FFmpeg>();
    }
#endif
#if HAVE_SHMDATA
    else if (type == "image_shmdata")
    {
        source = make_shared<Image_Shmdata>();
    }
#endif
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

    _attribFunctions["pause"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        _paused = args[0].asInt();

        return true;
    }, [&]() -> Values {
        return {_paused};
    });
    _attribFunctions["pause"].doUpdateDistant(true);

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

    _attribFunctions["seek"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        int64_t seekTime = args[0].asFloat() * 1e6;
        _startTime = Timer::getTime<chrono::microseconds>() - seekTime;
        _seeked = true;
        return true;
    }, [&]() -> Values {
        return {(float)_currentTime / 1e6};
    });
    _attribFunctions["seek"].doUpdateDistant(true);

    _attribFunctions["useClock"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        _useClock = args[0].asInt();
        if (_currentSource)
            _currentSource->setAttribute("useClock", {_useClock});

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
            auto sourceName = _name + DISTANT_NAME_SUFFIX;

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
                auto image = make_shared<Image>();
                image->setTo(0.f);
                image->setRemoteType(type);

                object = image;
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
