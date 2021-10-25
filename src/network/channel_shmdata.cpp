#include "./network/channel_shmdata.h"

#include <cassert>
#include <chrono>
#include <thread>

#include "./core/constants.h"
#include "./core/root_object.h"
#include "./core/serialized_object.h"
#include "./utils/log.h"

namespace Splash
{

const size_t ChannelOutput_Shmdata::_shmDefaultSize = 1 << 10;
const std::string ChannelOutput_Shmdata::_defaultPathPrefix = "/tmp/splash_";

/*************/
ChannelOutput_Shmdata::ChannelOutput_Shmdata(const RootObject* root, const std::string& name)
    : ChannelOutput(root, name)
{
    assert(_root != nullptr);
    const auto socketPrefix = _root->getSocketPrefix();
    _pathPrefix = _defaultPathPrefix;
    if (!socketPrefix.empty())
        _pathPrefix += socketPrefix + std::string("_");

    _ready = true;

    _msgConsumeThread = std::thread([&]() { messageConsume(); });
    _bufConsumeThread = std::thread([&]() { bufferConsume(); });

    _msgWriter = std::make_unique<shmdata::Writer>(_pathPrefix + "msg_" + _name, _shmDefaultSize, "splash/x-msg", &_shmlogger, [&](int /*id*/) {
        std::unique_lock<std::mutex> lock(_worldConnectedMutex);
        _isWorldConnected = true;
        _worldConnectedCondition.notify_one();
    });
    _bufWriter = std::make_unique<shmdata::Writer>(_pathPrefix + "buf_" + _name, _shmDefaultSize, "splash/x-msg", &_shmlogger);
}

/*************/
ChannelOutput_Shmdata::~ChannelOutput_Shmdata()
{
    _joinAllThreads = true;
    if (_msgConsumeThread.joinable())
        _msgConsumeThread.join();
    if (_bufConsumeThread.joinable())
        _bufConsumeThread.join();
}

/*************/
bool ChannelOutput_Shmdata::connectTo(const std::string& target)
{
    if (target == "world")
    {
        std::unique_lock<std::mutex> lock(_worldConnectedMutex);
        // Connection to the world should be mostly immediate, so waiting 5 seconds
        // looks like a correct failsafe
        _worldConnectedCondition.wait_for(lock, std::chrono::seconds(Constants::CONNECTION_TIMEOUT));
        if (!_isWorldConnected)
            return false;
    }

    return true;
}

/*************/
bool ChannelOutput_Shmdata::sendMessage(const std::vector<uint8_t>& message)
{
    std::unique_lock<std::mutex> lock(_msgConsumeMutex);
    _msgQueue.push_back(message);
    _msgNewInQueue = true;
    _msgCondition.notify_one();
    return true;
}

/*************/
bool ChannelOutput_Shmdata::sendBuffer(SerializedObject&& buffer)
{
    std::unique_lock<std::mutex> lock(_bufConsumeMutex);
    _bufQueue.push_back(std::move(buffer));
    _bufNewInQueue = true;
    _bufCondition.notify_one();
    return true;
}

/*************/
void ChannelOutput_Shmdata::messageConsume()
{
    std::unique_lock<std::mutex> lock(_msgConsumeMutex);
    while (!_joinAllThreads)
    {
        _msgCondition.wait_for(lock, std::chrono::milliseconds(50));
        if (!_msgNewInQueue)
            continue;

        for (const auto& message : _msgQueue)
        {
            if (!_msgWriter->copy_to_shm(message.data(), message.size()))
                Log::get() << Log::WARNING << "ChannelOutput_Shmdata::messageConsume - Error while sending message\n";
        }
        _msgQueue.clear();
        _msgNewInQueue = false;
    }
}

/*************/
void ChannelOutput_Shmdata::bufferConsume()
{
    std::unique_lock<std::mutex> lock(_bufConsumeMutex);
    while (!_joinAllThreads)
    {
        _bufCondition.wait_for(lock, std::chrono::milliseconds(50));
        if (!_bufNewInQueue)
            continue;

        for (auto& buffer : _bufQueue)
        {
            if (!_bufWriter->copy_to_shm(buffer.data(), buffer.size()))
                Log::get() << Log::WARNING << "ChannelOutput_Shmdata::messageConsume - Error while sending message\n";
        }
        _bufQueue.clear();
        _bufNewInQueue = false;
    }
}

const std::string ChannelInput_Shmdata::_defaultPathPrefix = "/tmp/splash_";

/*************/
ChannelInput_Shmdata::ChannelInput_Shmdata(const RootObject* root, const std::string& name, const MessageRecvCallback& msgRecvCb, const BufferRecvCallback& bufferRecvCb)
    : ChannelInput(root, name, msgRecvCb, bufferRecvCb)
{
    assert(_root != nullptr);
    const auto socketPrefix = _root->getSocketPrefix();
    _pathPrefix = _defaultPathPrefix;
    if (!socketPrefix.empty())
        _pathPrefix += socketPrefix + std::string("_");

    _ready = true;

    _msgConsumeThread = std::thread([&]() { messageConsume(); });
    _bufConsumeThread = std::thread([&]() { bufferConsume(); });
}

/*************/
ChannelInput_Shmdata::~ChannelInput_Shmdata()
{
    _joinAllThreads = true;
    if (_msgConsumeThread.joinable())
        _msgConsumeThread.join();
    if (_bufConsumeThread.joinable())
        _bufConsumeThread.join();
}

/*************/
bool ChannelInput_Shmdata::connectTo(const std::string& target)
{
    if (_msgFollowers.find(target) != _msgFollowers.end())
    {
        assert(_bufFollowers.find(target) != _bufFollowers.end());
        return false;
    }
    assert(_bufFollowers.find(target) == _bufFollowers.end());

    _msgFollowers.insert({target,
        std::make_unique<shmdata::Follower>(
            _pathPrefix + "msg_" + target,
            [&](void* data, size_t size) {
                std::unique_lock<std::mutex> lock(_msgConsumeMutex);
                _msgQueue.emplace_back(reinterpret_cast<uint8_t*>(data), reinterpret_cast<uint8_t*>(data) + size);
                _msgNewInQueue = true;
                _msgCondition.notify_one();
            },
            [&](const std::string& caps) { _msgCaps = caps; },
            [&]() {},
            &_shmlogger)});

    _bufFollowers.insert({target,
        std::make_unique<shmdata::Follower>(
            _pathPrefix + "buf_" + target,
            [&](void* data, size_t size) {
                std::unique_lock<std::mutex> lock(_bufConsumeMutex);
                _bufQueue.emplace_back(std::vector<uint8_t>(reinterpret_cast<uint8_t*>(data), reinterpret_cast<uint8_t*>(data) + size));
                _bufNewInQueue = true;
                _bufCondition.notify_one();
            },
            [&](const std::string& caps) { _bufCaps = caps; },
            [&]() {},
            &_shmlogger)});

    return true;
}

/*************/
void ChannelInput_Shmdata::messageConsume()
{
    while (!_joinAllThreads)
    {
        std::vector<std::vector<uint8_t>> localQueue;
        bool updatedQueue = false;

        {
            std::unique_lock<std::mutex> lock(_msgConsumeMutex);
            _msgCondition.wait_for(lock, std::chrono::milliseconds(50));
            if (_msgNewInQueue)
            {
                // std::swap(_msgQueue, localQueue);
                localQueue = _msgQueue;
                _msgQueue.clear();
                updatedQueue = true;
                _msgNewInQueue = false;
            }
        }

        if (updatedQueue)
        {
            for (const auto& message : localQueue)
                _msgRecvCb(message);
        }
    }
}

/*************/
void ChannelInput_Shmdata::bufferConsume()
{
    while (!_joinAllThreads)
    {
        std::vector<SerializedObject> localQueue;
        bool updatedQueue = false;

        {
            std::unique_lock<std::mutex> lock(_bufConsumeMutex);
            _bufCondition.wait_for(lock, std::chrono::milliseconds(50));
            if (_bufNewInQueue)
            {
                std::swap(_bufQueue, localQueue);
                updatedQueue = true;
                _bufNewInQueue = false;
            }
        }

        if (updatedQueue)
        {
            for (auto& buffer : localQueue)
                _bufferRecvCb(std::move(buffer));
        }
    }
}

/*************/
bool ChannelInput_Shmdata::disconnectFrom(const std::string& target)
{
    if (_msgFollowers.find(target) == _msgFollowers.end())
    {
        assert(_bufFollowers.find(target) == _bufFollowers.end());
        return false;
    }
    assert(_bufFollowers.find(target) != _bufFollowers.end());

    _msgFollowers.erase(target);
    _bufFollowers.erase(target);

    return true;
}

} // namespace Splash
