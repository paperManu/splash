#include "./image/queue.h"

#include <algorithm>

#include "./utils/log.h"
#include "./utils/timer.h"

#define DISTANT_NAME_SUFFIX "_queue_source"

namespace chrono = std::chrono;

namespace Splash
{

/*************/
Queue::Queue(RootObject* root)
    : BufferObject(root)
    , _factory(std::make_unique<Factory>(_root))
{
    _type = "queue";

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    registerAttributes();
}

/*************/
Queue::~Queue() {}

/*************/
SerializedObject Queue::serialize() const
{
    if (_currentSource)
        return _currentSource->serialize();
    else
        return {};
}

/*************/
std::string Queue::getDistantName() const
{
    return _name + DISTANT_NAME_SUFFIX;
}

/*************/
void Queue::update()
{
    std::lock_guard<std::mutex> lock(_playlistMutex);

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
        const auto previousTime = _currentTime;
        _currentTime = Timer::getTime() - _startTime;

        if (_paused)
        {
            _startTime = _startTime + (_currentTime - previousTime);
            _currentTime = previousTime;
            if (!_playlist[_currentSourceIndex].freeRun)
            {
                if (_currentSource)
                    _currentSource->setAttribute("pause", {true});
                return;
            }
        }
        else if (_currentSource)
            _currentSource->setAttribute("pause", {false});
    }

    // Get the current index regarding the current time
    uint32_t sourceIndex = 0;
    for (const auto& playSource : _playlist)
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
    if (sourceIndex != static_cast<uint32_t>(_currentSourceIndex))
    {
        if (_playing)
        {
            Log::get() << Log::MESSAGE << "Queue::" << __FUNCTION__ << " - Finished playing file: " << _playlist[_currentSourceIndex].filename << Log::endl;
            _playing = false;
        }

        _currentSourceIndex = sourceIndex;

        // If we are past the last index, we create a blank image
        if (sourceIndex >= _playlist.size())
        {
            _currentSource = std::dynamic_pointer_cast<BufferObject>(_factory->create("image"));
            _currentSource->setName(_name + DISTANT_NAME_SUFFIX);
            _root->sendMessage(_name, "source", {"image"});
        }
        // Otherwise we create the new source
        else
        {
            const auto& sourceParameters = _playlist[_currentSourceIndex];
            _currentSource = std::dynamic_pointer_cast<BufferObject>(_factory->create(sourceParameters.type));

            if (_currentSource)
                _playing = true;
            else
                _currentSource = std::dynamic_pointer_cast<BufferObject>(_factory->create("image"));

            std::dynamic_pointer_cast<Image>(_currentSource)->zero();
            _currentSource->setName(_name + DISTANT_NAME_SUFFIX);
            _currentSource->setAttribute("file", {sourceParameters.filename});

            if (_useClock && !sourceParameters.freeRun)
            {
                // If we use the master clock, set a timeshift to be correctly placed in the video
                // (as the source gets its clock from the same Timer)
                _currentSource->setAttribute("timeShift", {-static_cast<float>(sourceParameters.start) / 1e6});
                _currentSource->setAttribute("useClock", {true});
            }
            else
            {
                _currentSource->setAttribute("useClock", {false});
            }

            _root->sendMessage(_name, "source", {sourceParameters.type});

            for (const auto& arg : sourceParameters.args)
            {
                if (!arg.isNamed())
                    continue;

                _currentSource->setAttribute(arg.getName(), arg.as<Values>());
            }

            Log::get() << Log::MESSAGE << "Queue::" << __FUNCTION__ << " - Playing source: " << sourceParameters.filename << Log::endl;
        }
    }

    if (!_useClock && !_playlist[_currentSourceIndex].freeRun && _seeked)
    {
        // If we don't use the master clock, we want to seek accordingly in the file
        _currentSource->setAttribute("seek", {static_cast<float>(_currentTime - _playlist[_currentSourceIndex].start) / 1e6});
        _seeked = false;
    }

    if (_currentSource)
        _currentSource->update();
}

/*************/
void Queue::cleanPlaylist(std::vector<Source>& playlist)
{
    auto cleanList = std::vector<Source>();

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
        if (source.type != "image_ffmpeg")
            continue;

        if (source.stop > source.start)
            continue;

        auto videoSrc = _factory->create("image_ffmpeg");
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
            if (!cleanList.empty() && source.filename == "black")
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
            else if (!cleanList.empty() && cleanList.back().filename == "black")
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
void Queue::runTasks()
{
    BaseObject::runTasks();
    if (_currentSource != nullptr)
        _currentSource->runTasks();
}

/*************/
void Queue::registerAttributes()
{
    BufferObject::registerAttributes();

    addAttribute("loop",
        [&](const Values& args) {
            _loop = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_loop}; },
        {'b'});
    setAttributeDescription("loop", "Set whether to loop through the queue or not");

    addAttribute("pause",
        [&](const Values& args) {
            _paused = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_paused}; },
        {'b'});
    setAttributeDescription("pause", "Pause the queue if true");

    addAttribute("playlist",
        [&](const Values& args) {
            std::lock_guard<std::mutex> lock(_playlistMutex);
            std::vector<Source> playlist;

            for (auto& it : args)
            {
                auto src = it.as<Values>();

                if (src.size() == 6) // We need at least type, name, start and stop for each input
                {
                    Source source;
                    source.type = src[0].as<std::string>();
                    source.filename = src[1].as<std::string>();
                    source.start = static_cast<int64_t>(src[2].as<float>() * 1e6);
                    source.stop = static_cast<int64_t>(src[3].as<float>() * 1e6);
                    source.freeRun = src[4].as<bool>();
                    source.args = src[5].as<Values>();

                    if (source.start > source.stop && source.stop != 0.0)
                        return false;

                    playlist.push_back(source);
                }
                else
                {
                    Log::get() << Log::WARNING << "Queue~~playlist - Wrong number of arguments given for one of the sources" << Log::endl;
                }
            }

            cleanPlaylist(playlist);
            _playlist = playlist;

            return true;
        },
        [&]() -> Values {
            std::lock_guard<std::mutex> lock(_playlistMutex);
            Values playlist;

            for (auto& src : _playlist)
            {
                Values source;
                source.push_back(src.type);
                source.push_back(src.filename);
                source.push_back(static_cast<double>(src.start) / 1e6);
                source.push_back(static_cast<double>(src.stop) / 1e6);
                source.push_back(static_cast<int>(src.freeRun));
                source.push_back(src.args);

                playlist.emplace_back(std::move(source));
            }

            return playlist;
        },
        {});
    setAttributeDescription("playlist", "Set the playlist as an array of [type, filename, start, end, (args)]");

    addAttribute(
        "elapsed", [&](const Values& /*args*/) { return true; }, [&]() -> Values { return {static_cast<float>(_currentTime / 1e6)}; }, {'r'});
    setAttributeDescription("elapsed", "Time elapsed since the beginning of the queue");

    addAttribute("seek",
        [&](const Values& args) {
            _seekTime = args[0].as<float>();
            _startTime = Timer::getTime() - static_cast<int64_t>(_seekTime * 1e6);
            _seeked = true;
            return true;
        },
        [&]() -> Values { return {_seekTime}; },
        {'r'});
    setAttributeDescription("seek", "Seek through the playlist");

    addAttribute("useClock",
        [&](const Values& args) {
            _useClock = args[0].as<bool>();
            if (_currentSource)
                _currentSource->setAttribute("useClock", {_useClock});

            return true;
        },
        [&]() -> Values { return {_useClock}; },
        {'b'});
    setAttributeDescription("useClock", "Use the master clock if true");
}

/*************/
/*************/

int QueueSurrogate::_filterIndex = 0;

/*************/
QueueSurrogate::QueueSurrogate(RootObject* root)
    : Texture(root)
    , _filter(std::make_shared<Filter>(root))
{
    _filter = std::dynamic_pointer_cast<Filter>(_root->createObject("filter", "queueFilter_" + _name + std::to_string(_filterIndex++)).lock());
    _filter->setAttribute("savable", {false});

    registerAttributes();
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
std::unordered_map<std::string, Values> QueueSurrogate::getShaderUniforms() const
{
    return _filter->getShaderUniforms();
}

/*************/
ImageBufferSpec QueueSurrogate::getSpec() const
{
    return _filter->getSpec();
}

/*************/
void QueueSurrogate::registerAttributes()
{
    Texture::registerAttributes();

    /*
     * Create the object for the current source type
     * Args holds the object type (Image, Texture...)
     */
    addAttribute("source",
        [&](const Values& args) {
            if (args.size() != 1)
                return false;

            addTask([=, this]() {
                auto sourceName = _name + DISTANT_NAME_SUFFIX;
                auto type = args[0].as<std::string>();

                if (_source && type.find(_source->getType()) != std::string::npos)
                {
                    return;
                }
                else if (_source)
                {
                    _filter->unlinkFrom(_source);
                    _source.reset();
                    _root->disposeObject(_name + "_source");
                }

                auto object = std::shared_ptr<GraphObject>();

                if (type.find("image") != std::string::npos)
                {
                    auto image = std::dynamic_pointer_cast<Image>(_root->createObject("image", _name + DISTANT_NAME_SUFFIX).lock());
                    image->zero();
                    image->setRemoteType(type);
                    object = image;
                }
                else
                {
                    return;
                }

                object->setName(sourceName);
                object->setAttribute("savable", {false});
                _source.swap(object);
                _filter->linkTo(_source);
            });

            return true;
        },
        {});
}

} // namespace Splash
