#include "queue.h"

#include <algorithm>

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
Queue::Queue(std::weak_ptr<RootObject> root)
    : BufferObject(root)
{
    _type = "queue";
    registerAttributes();

    // If the root object weak_ptr is expired, this means that
    // this object has been created outside of a World or Scene.
    // This is used for getting documentation "offline"
    if (_root.expired())
        return;

    _world = dynamic_pointer_cast<World>(root.lock());
}

/*************/
Queue::~Queue()
{
}

/*************/
shared_ptr<SerializedObject> Queue::serialize() const
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
    lock_guard<mutex> lock(_playlistMutex);

    if (_playlist.size() == 0)
        return;

    if (_startTime < 0)
        _startTime = Timer::getTime();

    int64_t masterClockTime;
    bool masterClockPaused;
    if (_useClock && Timer::get().getMasterClock<chrono::microseconds>(masterClockTime, masterClockPaused))
    {
        _currentTime = masterClockTime;
    }
    else
    {
        auto previousTime = _currentTime;
        _currentTime = Timer::getTime() - _startTime;

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
        _startTime = Timer::getTime();
        _currentTime = 0;
    }

    // If the index changed
    if (sourceIndex != _currentSourceIndex)
    {
        if (_playing)
        {
            Log::get() << Log::MESSAGE << "Queue::" << __FUNCTION__ << " - Finished playing file: " << _playlist[_currentSourceIndex].filename << Log::endl;
            _playing = false;
        }

        _currentSourceIndex = sourceIndex;
        _currentSource.reset();

        if (sourceIndex >= _playlist.size())
        {
            _currentSource = make_shared<Image>(_root);
            _world.lock()->sendMessage(_name, "source", {"image"});
        }
        else
        {
            _currentSource = createSource(_playlist[_currentSourceIndex].type);

            if (_currentSource)
                _playing = true;
            else
                _currentSource = make_shared<Image>(_root);

            _currentSource->setAttribute("file", {_playlist[_currentSourceIndex].filename});

            if (_useClock)
            {
                // If we use the master clock, set a timeshift to be correctly placed in the video
                // (as the source gets its clock from the same Timer)
                _currentSource->setAttribute("timeShift", {-(float)_playlist[_currentSourceIndex].start / 1e6});
                _currentSource->setAttribute("useClock", {1});
            }

            _world.lock()->sendMessage(_name, "source", {_playlist[_currentSourceIndex].type});

            Log::get() << Log::MESSAGE << "Queue::" << __FUNCTION__ << " - Playing file: " << _playlist[_currentSourceIndex].filename << Log::endl;
        }
    }

    if (!_useClock && _seeked)
    {
        // If we don't use the master clock, we want to seek accordingly in the file
        _currentSource->setAttribute("seek", {(float)(_currentTime - _playlist[_currentSourceIndex].start) / 1e6});
        _seeked = false;
    }

    if (_currentSource)
        _currentSource->update();
}

/*************/
void Queue::cleanPlaylist(vector<Source>& playlist)
{
    auto cleanList = vector<Source>();

    std::sort(playlist.begin(), playlist.end(), [](const Source& a, const Source& b) { return a.start < b.start; });

    // Clean each individual source
    for (auto it = playlist.begin(); it != playlist.end();)
    {
        if (it->start >= it->stop && it->stop != 0l)
        {
            if (it->stop > 1000000l)
                it->start = it->stop - 1000000l;
        }
        else
        {
            it++;
        }
    }

    // Find duration for videos with stop == 0
    for (auto& source : playlist)
    {
        if (source.stop > source.start)
            continue;

        auto videoSrc = unique_ptr<Image_FFmpeg>(new Image_FFmpeg(_root));
        videoSrc->setAttribute("file", {source.filename});
        Values duration;
        videoSrc->getAttribute("duration", duration);
        if (duration.size() > 0)
            source.stop = duration[0].as<long>() * 1e6 + source.start;
    }

    // Clean the queue, add black intermediate images, ...
    int64_t previousEnd = 0;
    for (const auto& source : playlist)
    {
        if (previousEnd < source.start)
        {
            if (source.filename == "black")
            {
                if (cleanList.back().filename == "black")
                {
                    cleanList.back().stop = source.stop;
                }
                else
                {
                    cleanList.push_back(source);
                    cleanList.back().start = previousEnd;
                }
            }
            else if (cleanList.size() > 0 && cleanList.back().filename == "black")
            {
                cleanList.back().stop = source.start;
                cleanList.push_back(source);
            }
            else
            {
                Source blackSource;
                blackSource.start = previousEnd;
                blackSource.stop = source.start;
                blackSource.type = "image";
                blackSource.filename = "black";
                cleanList.push_back(blackSource);
                cleanList.push_back(source);
            }
        }
        else if (previousEnd == source.start)
        {
            cleanList.push_back(source);
        }
        else if (previousEnd > source.start)
        {
            if (source.filename == "black")
            {
                cleanList.push_back(source);
                cleanList.back().start = previousEnd;
            }
            else
            {
                cleanList.back().stop = source.start;
                cleanList.push_back(source);
            }
        }

        previousEnd = source.stop;
    }

    // Remove zero length black sources
    for (auto it = cleanList.begin(); it != cleanList.end();)
    {
        if (it->start >= it->stop && it->filename == "black")
            it = cleanList.erase(it);
        else
            it++;
    }

    playlist = cleanList;
}

/*************/
shared_ptr<BufferObject> Queue::createSource(string type)
{
    auto source = shared_ptr<BufferObject>();

    if (type == "image")
    {
        source = make_shared<Image>(_root);
        dynamic_pointer_cast<Image>(source)->setTo(0.f);
    }
#if HAVE_FFMPEG
    else if (type == "image_ffmpeg")
    {
        source = make_shared<Image_FFmpeg>(_root);
        dynamic_pointer_cast<Image>(source)->setTo(0.f);
    }
#endif
#if HAVE_SHMDATA
    else if (type == "image_shmdata")
    {
        source = make_shared<Image_Shmdata>(_root);
        dynamic_pointer_cast<Image>(source)->setTo(0.f);
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
    addAttribute("loop",
        [&](const Values& args) {
            _loop = (bool)args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {_loop}; },
        {'n'});
    setAttributeParameter("loop", true, true);
    setAttributeDescription("loop", "Set whether to loop through the queue or not");

    addAttribute("pause",
        [&](const Values& args) {
            _paused = args[0].as<int>();

            return true;
        },
        [&]() -> Values { return {_paused}; },
        {'n'});
    setAttributeParameter("pause", false, true);
    setAttributeDescription("pause", "Pause the queue if set to 1");

    addAttribute("playlist",
        [&](const Values& args) {
            lock_guard<mutex> lock(_playlistMutex);
            _playlist.clear();

            for (auto& it : args)
            {
                auto src = it.as<Values>();

                if (src.size() >= 4) // We need at least type, name, start and stop for each input
                {
                    Source source;
                    source.type = src[0].as<string>();
                    source.filename = src[1].as<string>();
                    source.start = (int64_t)(src[2].as<float>() * 1e6);
                    source.stop = (int64_t)(src[3].as<float>() * 1e6);
                    for (auto idx = 4; idx < src.size(); ++idx)
                        source.args.push_back(src[idx]);

                    _playlist.push_back(source);
                }
            }

            cleanPlaylist(_playlist);

            return true;
        },
        [&]() -> Values {
            lock_guard<mutex> lock(_playlistMutex);
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
    setAttributeParameter("playlist", true, true);
    setAttributeDescription("playlist", "Set the playlist as an array of [type, filename, start, end, (args)]");

    addAttribute("seek",
        [&](const Values& args) {
            int64_t seekTime = args[0].as<float>() * 1e6;
            _startTime = Timer::getTime() - seekTime;
            _seeked = true;
            return true;
        },
        [&]() -> Values { return {(float)_currentTime / 1e6}; },
        {'n'});
    setAttributeParameter("seek", false, true);
    setAttributeDescription("seek", "Seek through the playlist");

    addAttribute("useClock",
        [&](const Values& args) {
            _useClock = args[0].as<int>();
            if (_currentSource)
                _currentSource->setAttribute("useClock", {_useClock});

            return true;
        },
        [&]() -> Values { return {(int)_useClock}; },
        {'n'});
    setAttributeParameter("useClock", true, true);
    setAttributeDescription("useClock", "Use the master clock if set to 1");
}

/*************/
/*************/

/*************/
QueueSurrogate::QueueSurrogate(std::weak_ptr<RootObject> root)
    : Texture(root)
{
    _type = "queue";
    _renderingPriority = Priority::PRE_CAMERA;
    _remoteType = "queue";
    _filter = make_shared<Filter>(root);
    _filter->setName("queueFilter" + to_string(_filterIndex++));
    _root.lock()->registerObject(_filter);
    _filter->_savable = false;

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
ImageBufferSpec QueueSurrogate::getSpec() const
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
    addAttribute("source", [&](const Values& args) {
        if (args.size() != 1)
            return false;

        lock_guard<mutex> lock(_taskMutex);
        _taskQueue.push_back([=]() {
            auto sourceName = _name + DISTANT_NAME_SUFFIX;
            auto type = args[0].as<string>();

            if (_source && type.find(_source->getType()) != string::npos)
            {
                return;
            }
            else if (_source)
            {
                _filter->unlinkFrom(_source);
                _root.lock()->unregisterObject(_source->getName());
                _source.reset();
            }

            auto object = shared_ptr<BaseObject>();

            if (type.find("image") != string::npos)
            {
                auto image = make_shared<Image>(_root);
                image->setTo(0.f);
                image->setRemoteType(type);

                object = image;
            }
            // TODO: add Texture_Syphon type
            // else if (type.find("texture_syphon") != string::npos)
            //{
            //    object = make_shared<Texture_Syphon>();
            //}
            else
            {
                return;
            }

            object->setName(sourceName);
            _source.swap(object);
            _root.lock()->registerObject(_source);
            _filter->linkTo(_source);
        });

        return true;
    });
}

} // end of namespace
