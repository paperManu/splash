#include "./image/queue.h"

#include <algorithm>

#include "./utils/log.h"
#include "./utils/timer.h"

#define DISTANT_NAME_SUFFIX "_source"

using namespace std;

namespace Splash
{

/*************/
Queue::Queue(RootObject* root)
    : BufferObject(root)
    , _factory(make_unique<Factory>(_root))
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
            if (!_playlist[_currentSourceIndex].freeRun)
            {
                if (_currentSource)
                    _currentSource->setAttribute("pause", {1});
                return;
            }
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
    if (sourceIndex != static_cast<uint32_t>(_currentSourceIndex))
    {
        if (_playing)
        {
            Log::get() << Log::MESSAGE << "Queue::" << __FUNCTION__ << " - Finished playing file: " << _playlist[_currentSourceIndex].filename << Log::endl;
            _playing = false;
        }

        _currentSourceIndex = sourceIndex;

        if (sourceIndex >= _playlist.size())
        {
            _currentSource = dynamic_pointer_cast<BufferObject>(_factory->create("image"));
            _root->sendMessage(_name, "source", {"image"});
        }
        else
        {
            auto& sourceParameters = _playlist[_currentSourceIndex];

            if (!_currentSource || _currentSource->getType() != sourceParameters.type)
                _currentSource = dynamic_pointer_cast<BufferObject>(_factory->create(sourceParameters.type));

            if (_currentSource)
                _playing = true;
            else
                _currentSource = dynamic_pointer_cast<BufferObject>(_factory->create("image"));
            dynamic_pointer_cast<Image>(_currentSource)->zero();
            dynamic_pointer_cast<Image>(_currentSource)->setName(_name + DISTANT_NAME_SUFFIX);

            _currentSource->setAttribute("file", {sourceParameters.filename});

            if (_useClock && !sourceParameters.freeRun)
            {
                // If we use the master clock, set a timeshift to be correctly placed in the video
                // (as the source gets its clock from the same Timer)
                _currentSource->setAttribute("timeShift", {-(float)sourceParameters.start / 1e6});
                _currentSource->setAttribute("useClock", {1});
            }
            else
            {
                _currentSource->setAttribute("useClock", {0});
            }

            _root->sendMessage(_name, "source", {sourceParameters.type});

            for (const auto& arg : sourceParameters.args)
            {
                if (!arg.isNamed())
                    continue;

                _currentSource->setAttribute(arg.getName(), arg.as<Values>());
            }

            Log::get() << Log::MESSAGE << "Queue::" << __FUNCTION__ << " - Playing file: " << sourceParameters.filename << Log::endl;
        }
    }

    if (!_useClock && !_playlist[_currentSourceIndex].freeRun && _seeked)
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
void Queue::registerAttributes()
{
    BufferObject::registerAttributes();

    addAttribute("loop",
        [&](const Values& args) {
            _loop = (bool)args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {_loop}; },
        {'n'});
    setAttributeParameter("loop", true);
    setAttributeDescription("loop", "Set whether to loop through the queue or not");

    addAttribute("pause",
        [&](const Values& args) {
            _paused = args[0].as<int>();

            return true;
        },
        [&]() -> Values { return {_paused}; },
        {'n'});
    setAttributeParameter("pause", false);
    setAttributeDescription("pause", "Pause the queue if set to 1");

    addAttribute("playlist",
        [&](const Values& args) {
            lock_guard<mutex> lock(_playlistMutex);
            vector<Source> playlist;

            for (auto& it : args)
            {
                auto src = it.as<Values>();

                if (src.size() == 6) // We need at least type, name, start and stop for each input
                {
                    Source source;
                    source.type = src[0].as<string>();
                    source.filename = src[1].as<string>();
                    source.start = (int64_t)(src[2].as<float>() * 1e6);
                    source.stop = (int64_t)(src[3].as<float>() * 1e6);
                    source.freeRun = src[4].as<bool>();
                    source.args = src[5].as<Values>();

                    if (source.start > source.stop)
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
            lock_guard<mutex> lock(_playlistMutex);
            Values playlist;

            for (auto& src : _playlist)
            {
                Values source;
                source.push_back(src.type);
                source.push_back(src.filename);
                source.push_back((double)src.start / 1e6);
                source.push_back((double)src.stop / 1e6);
                source.push_back((int)src.freeRun);
                source.push_back(src.args);

                playlist.emplace_back(std::move(source));
            }

            return playlist;
        });
    setAttributeParameter("playlist", true);
    setAttributeDescription("playlist", "Set the playlist as an array of [type, filename, start, end, (args)]");

    addAttribute("elapsed", [&](const Values& /*args*/) { return true; }, [&]() -> Values { return {static_cast<float>(_currentTime / 1e6)}; }, {'n'});
    setAttributeDescription("elapsed", "Time elapsed since the beginning of the queue");

    addAttribute("seek",
        [&](const Values& args) {
            _seekTime = args[0].as<float>();
            _startTime = Timer::getTime() - static_cast<int64_t>(_seekTime * 1e6);
            _seeked = true;
            return true;
        },
        [&]() -> Values { return {_seekTime}; },
        {'n'});
    setAttributeParameter("seek", false);
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
    setAttributeParameter("useClock", true);
    setAttributeDescription("useClock", "Use the master clock if set to 1");
}

/*************/
/*************/

/*************/
QueueSurrogate::QueueSurrogate(RootObject* root)
    : Texture(root)
    , _filter(make_shared<Filter>(root))
{
    _filter = dynamic_pointer_cast<Filter>(_root->createObject("filter", "queueFilter_" + _name + to_string(_filterIndex++)).lock());
    _filter->_savable = false;

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
void QueueSurrogate::registerAttributes()
{
    Texture::registerAttributes();

    /*
     * Create the object for the current source type
     * Args holds the object type (Image, Texture...)
     */
    addAttribute("source", [&](const Values& args) {
        if (args.size() != 1)
            return false;

        addTask([=]() {
            auto sourceName = _name + DISTANT_NAME_SUFFIX;
            auto type = args[0].as<string>();

            if (_source && type.find(_source->getType()) != string::npos)
            {
                return;
            }
            else if (_source)
            {
                _filter->unlinkFrom(_source);
                _source.reset();
                _root->disposeObject(_name + "_source");
            }

            auto object = shared_ptr<GraphObject>();

            if (type.find("image") != string::npos)
            {
                auto image = dynamic_pointer_cast<Image>(_root->createObject("image", _name + "_source").lock());
                image->zero();
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
            _filter->linkTo(_source);
        });

        return true;
    });
}

} // namespace Splash
