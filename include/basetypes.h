/*
 * Copyright (C) 2015 Emmanuel Durand
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @basetypes.h
 * Base types, from which more complex classes derive
 */

#ifndef SPLASH_BASETYPES_H
#define SPLASH_BASETYPES_H

#include <atomic>
#include <condition_variable>
#include <list>
#include <map>
#include <unordered_map>
#include <json/json.h>

#include "coretypes.h"
#include "link.h"
#include "log.h"
#include "timer.h"

namespace Splash
{

/*************/
struct AttributeFunctor
{
    public:
        AttributeFunctor() {};

        AttributeFunctor(const std::string& name, std::function<bool(const Values&)> setFunc, const std::vector<char>& types = {})
        {
            _name = name;
            _setFunc = setFunc;
            _getFunc = std::function<const Values()>();
            _defaultSetAndGet = false;
            _valuesTypes = types;
        }
        
        AttributeFunctor(const std::string& name,
                         std::function<bool(const Values&)> setFunc,
                         std::function<const Values()> getFunc,
                         const std::vector<char>& types = {})
        {
            _name = name;
            _setFunc = setFunc;
            _getFunc = getFunc;
            _defaultSetAndGet = false;
            _valuesTypes = types;
        }

        AttributeFunctor(const AttributeFunctor&) = delete;
        AttributeFunctor& operator=(const AttributeFunctor&) = delete;

        AttributeFunctor(AttributeFunctor&& a)
        {
            operator=(std::move(a));
        }

        AttributeFunctor& operator=(AttributeFunctor&& a)
        {
            if (this != &a)
            {
                _name = std::move(a._name);
                _objectName = std::move(a._objectName);
                _setFunc = std::move(a._setFunc);
                _getFunc = std::move(a._getFunc);
                _description = std::move(a._description);
                _values = std::move(a._values);
                _valuesTypes = std::move(a._valuesTypes);
                _defaultSetAndGet = std::move(a._defaultSetAndGet);
                _doUpdateDistant = std::move(a._doUpdateDistant);
                _savable = std::move(a._savable);
            }

            return *this;
        }

        bool operator()(const Values& args)
        {
            if (_isLocked)
                return false;

            if (!_setFunc && _defaultSetAndGet)
            {
                std::lock_guard<std::mutex> lock(_defaultFuncMutex);
                _values = args;

                _valuesTypes.clear();
                for (const auto& a : args)
                    _valuesTypes.push_back(a.getTypeAsChar());

                return true;
            }
            else if (!_setFunc)
            {
                return false;
            }

            // Check for arguments correctness
            // Some attributes may have an unlimited number of arguments, so we do not test for equality
            if (args.size() < _valuesTypes.size())
            {
                Log::get() << Log::WARNING << _objectName << "~~" << _name << " - Wrong number of arguments (" << args.size() << " instead of " << _valuesTypes.size() << ")" << Log::endl;
                return false;
            }

            for (int i = 0; i < _valuesTypes.size(); ++i)
            {
                auto type = args[i].getTypeAsChar();
                auto expected = _valuesTypes[i];

                if (type != expected)
                {
                    Log::get() << Log::WARNING << _objectName << "~~" << _name << " - Argument " << i << " is of wrong type " << std::string(&type, &type + 1) << ", expected " << std::string(&expected, &expected + 1) << Log::endl;
                    return false;
                }
            }

            return _setFunc(std::forward<const Values&>(args));
        }

        Values operator()() const
        {
            if (!_getFunc && _defaultSetAndGet)
            {
                std::lock_guard<std::mutex> lock(_defaultFuncMutex);
                return _values;
            }
            else if (!_getFunc)
            {
                return Values();
            }

            return _getFunc();
        }

        bool isDefault() const
        {
            return _defaultSetAndGet;
        }

        // Set whether to update the Scene object (if this attribute is hosted by a World object)
        bool doUpdateDistant() const {return _doUpdateDistant;}
        void doUpdateDistant(bool update) {_doUpdateDistant = update;}

        // Get the types of the wanted arguments
        Values getArgsTypes() const
        {
            Values types {};
            for (const auto& type : _valuesTypes)
                types.push_back(Value(std::string(&type, 1)));
            return types;
        }

        // Lock the attribute to the given value
        bool isLocked() const {return _isLocked;}
        bool lock(Values v = {})
        {
            if (v.size() != 0)
                if (!operator()(v))
                    return false;

            _isLocked = true;
            return true;
        }
        void unlock() {_isLocked = false;}

        // Savability (as JSON) of this attribute
        bool savable() const {return _savable;}
        void savable(bool save) {_savable = save;}

        // Description
        void setDescription(const std::string& desc) {_description = desc;}
        std::string getDescription() const {return _description;}

        // Name of the host object
        void setObjectName(const std::string& objectName) {_objectName = objectName;}

    private:
        mutable std::mutex _defaultFuncMutex {};
        std::function<bool(const Values&)> _setFunc {};
        std::function<const Values()> _getFunc {};

        std::string _objectName; // Name of the object holding this attribute
        std::string _name; // Name of the attribute
        std::string _description {}; // Attribute description
        Values _values; // Holds the values for the default set and get functions
        std::vector<char> _valuesTypes; // List of the types held in _values

        bool _isLocked {false};

        bool _defaultSetAndGet {true};
        bool _doUpdateDistant {false}; // True if the World should send this attr values to Scenes
        bool _savable {true}; // True if this attribute should be saved
};

class BaseObject;
typedef std::shared_ptr<BaseObject> BaseObjectPtr;
class RootObject;
typedef std::weak_ptr<RootObject> RootObjectWeakPtr;

/*************/
class BaseObject
{
    public:
        BaseObject()
        {
            init();
        }
        BaseObject(RootObjectWeakPtr root)
        {
            init();
            _root = root;
        }
        virtual ~BaseObject() {}

        /**
         * Safe bool idiom
         */
        virtual explicit operator bool() const
        {
            return true;
        }

        /**
         * Access the attributes through operator[]
         */
        AttributeFunctor& operator[](const std::string& attr)
        {
            auto attribFunction = _attribFunctions.find(attr);
            return attribFunction->second;
        }

        inline std::string getType() const {return _type;}

        /**
         * Set and get the id of the object
         */
        inline unsigned long getId() const {return _id;}
        inline void setId(unsigned long id) {_id = id;}

        /**
         * Set and get the name of the object
         */
        inline std::string getName() const {return _name;}
        inline virtual std::string setName(const std::string& name) {_name = name; return _name;}

        /**
         * Set and get the remote type of the object
         * This implies that this object gets data streamed from a World object
         */
        inline std::string getRemoteType() const {return _remoteType;}
        inline void setRemoteType(std::string type)
        {
            _remoteType = type;
            _isConnectedToRemote = true;
        }

        /**
         * Try to link / unlink the given BaseObject to this
         */
        virtual bool linkTo(std::shared_ptr<BaseObject> obj)
        {
            auto objectIt = std::find_if(_linkedObjects.begin(), _linkedObjects.end(), [&](const std::weak_ptr<BaseObject>& o) {
                auto object = o.lock();
                if (!object)
                    return false;
                if (object == obj)
                    return true;
                return false;
            });

            if (objectIt == _linkedObjects.end())
            {
                _linkedObjects.push_back(obj);
                return true;
            }
            return false;
        }

        /**
         * Unlink a given object
         */
        virtual void unlinkFrom(std::shared_ptr<BaseObject> obj)
        {
            auto objectIt = std::find_if(_linkedObjects.begin(), _linkedObjects.end(), [&](const std::weak_ptr<BaseObject>& o) {
                auto object = o.lock();
                if (!object)
                    return false;
                if (object == obj)
                    return true;
                return false;
            });

            if (objectIt != _linkedObjects.end())
                _linkedObjects.erase(objectIt);
        }

        /**
         * Return a vector of the linked objects
         */
        const std::vector<std::shared_ptr<BaseObject>> getLinkedObjects()
        {
            std::vector<std::shared_ptr<BaseObject>> objects;
            for (auto& o : _linkedObjects)
            {
                auto obj = o .lock();
                if (!obj)
                    continue;

                objects.push_back(obj);
            }

            return objects;
        }

        /**
         * Set the specified attribute
         * \params attrib Attribute name
         * \params args Values object which holds attribute values
         */
        bool setAttribute(const std::string& attrib, const Values& args)
        {
            auto attribFunction = _attribFunctions.find(attrib);
            bool attribNotPresent = (attribFunction == _attribFunctions.end());

            if (attribNotPresent)
            {
                auto result = _attribFunctions.emplace(attrib, AttributeFunctor());
                if (!result.second)
                    return false;

                attribFunction = result.first;
            }

            if (!attribFunction->second.isDefault())
                _updatedParams = true;
            bool attribResult = attribFunction->second(std::forward<const Values&>(args));

            return attribResult && attribNotPresent;
        }

        /**
         * Get the specified attribute
         * \params attrib Attribute name
         * \params args Values object which will hold the attribute values
         * \params includeDistant Return true even if the attribute is distant
         * \params includeNonSavable Return true even if the attribute is not savable
         */
        bool getAttribute(const std::string& attrib, Values& args, bool includeDistant = false, bool includeNonSavable = false) const
        {
            auto attribFunction = _attribFunctions.find(attrib);
            if (attribFunction == _attribFunctions.end())
                return false;

            args = attribFunction->second();

            if ((!attribFunction->second.savable() && !includeNonSavable)
             || (attribFunction->second.isDefault() && !includeDistant))
                return false;

            return true;
        }

        /**
         * Get all the savable attributes as a map
         * \params includeDistant Also include the distant attributes
         */
        std::unordered_map<std::string, Values> getAttributes(bool includeDistant = false) const
        {
            std::unordered_map<std::string, Values> attribs;
            for (auto& attr : _attribFunctions)
            {
                Values values;
                if (getAttribute(attr.first, values, includeDistant, true) == false || values.size() == 0)
                    continue;
                attribs[attr.first] = values;
            }

            return attribs;
        }

        /**
         * Get the map of the attributes which should be updated from World to Scene
         * This is the case when the distant object is different from the World one
         */
        std::unordered_map<std::string, Values> getDistantAttributes() const
        {
            std::unordered_map<std::string, Values> attribs;
            for (auto& attr : _attribFunctions)
            {
                if (!attr.second.doUpdateDistant())
                    continue;

                Values values;
                if (getAttribute(attr.first, values, false, true) == false || values.size() == 0)
                    continue;

                attribs[attr.first] = values;
            }

            return attribs;
        }

        /**
         * Get the savability for this object
         */
        inline bool getSavable() {return _savable;}

        /**
         * Check whether the objects needs to be updated
         */
        inline virtual bool wasUpdated() const {return _updatedParams;}

        /**
         * Reset the "was updated" status, if needed
         */
        inline virtual void setNotUpdated() {_updatedParams = false;}

        /**
         * Set the object savability
         */
        inline virtual void setSavable(bool savable) {_savable = savable;}
       
        /**
         * Update the content of the object
         */
        virtual void update() {}

        /**
         * Get the configuration as a json object
         */
        Json::Value getValuesAsJson(const Values& values) const
        {
            Json::Value jsValue;
            for (auto& v : values)
            {
                switch (v.getType())
                {
                default:
                    continue;
                case Value::i:
                    jsValue.append(v.asInt());
                    break;
                case Value::f:
                    jsValue.append(v.asFloat());
                    break;
                case Value::s:
                    jsValue.append(v.asString());
                    break;
                case Value::v:
                    jsValue.append(getValuesAsJson(v.asValues()));
                    break;
                }
            }
            return jsValue;
        }

        Json::Value getConfigurationAsJson() const
        {
            Json::Value root;
            if (_remoteType == "")
                root["type"] = _type;
            else
                root["type"] = _remoteType;

            for (auto& attr : _attribFunctions)
            {
                Values values;
                if (getAttribute(attr.first, values) == false || values.size() == 0)
                    continue;

                Json::Value jsValue;
                jsValue = getValuesAsJson(values);
                root[attr.first] = jsValue;
            }
            return root;
        }

        /**
         * Get the description for the given attribute, if it exists
         */
        std::string getAttributeDescription(const std::string& name)
        {
            auto attr = _attribFunctions.find(name);
            if (attr != _attribFunctions.end())
                return attr->second.getDescription();
            else
                return {};
        }

        /**
         * Get a Values holding the description of all of this object's attributes
         */
        Values getAttributesDescriptions()
        {
            Values descriptions;
            for (const auto& attr : _attribFunctions)
                descriptions.push_back(Values({attr.first, attr.second.getDescription(), attr.second.getArgsTypes()}));
            return descriptions;
        }

    // Pubic attributes
    public:
        bool _savable {true};

    protected:
        unsigned long _id;
        std::string _type {"baseobject"};
        std::string _remoteType {""};
        std::string _name {""};

        bool _isConnectedToRemote {false}; // True if the object gets data from a World object
        std::string _configFilePath {""}; // All objects know about their location

        RootObjectWeakPtr _root;
        std::vector<std::weak_ptr<BaseObject>> _linkedObjects;

        std::unordered_map<std::string, AttributeFunctor> _attribFunctions;
        bool _updatedParams {true};

        // Initialize generic attributes
        void init()
        {
            addAttribute("configFilePath", [&](const Values& args) {
                _configFilePath = args[0].asString();
                return true;
            }, {'s'});

            addAttribute("setName", [&](const Values& args) {
                setName(args[0].asString());
                return true;
            }, [&]() -> Values {
                return {_name};
            }, {'s'});

            addAttribute("switchLock", [&](const Values& args) {
                auto attribIterator = _attribFunctions.find(args[0].asString());
                if (attribIterator == _attribFunctions.end())
                    return false;

                std::string status;
                auto& attribFunctor = attribIterator->second;
                if (attribFunctor.isLocked())
                {
                    status = "Unlocked";
                    attribFunctor.unlock();
                }
                else
                {
                    status = "Locked";
                    attribFunctor.lock();
                }

                Log::get() << Log::MESSAGE << _name << "~~" << args[0].asString() << " - " << status << Log::endl;
                return true;
            }, {'s'});
        }

        /**
         * Add a new attribute to this object
         */
        AttributeFunctor& addAttribute(const std::string& name, std::function<bool(const Values&)> set, const std::vector<char> types = {})
        {
            _attribFunctions[name] = AttributeFunctor(name, set, types);
            _attribFunctions[name].setObjectName(_type);
            return _attribFunctions[name];
        }

        AttributeFunctor& addAttribute(const std::string& name, std::function<bool(const Values&)> set, std::function<const Values()> get, const std::vector<char>& types = {})
        {
            _attribFunctions[name] = AttributeFunctor(name, set, get, types);
            _attribFunctions[name].setObjectName(_type);
            return _attribFunctions[name];
        }

        /**
         * Set and the description for the given attribute, if it exists
         */
        void setAttributeDescription(const std::string& name, const std::string& description)
        {
            auto attr = _attribFunctions.find(name);
            if (attr != _attribFunctions.end())
            {
                attr->second.setDescription(description);
            }
        }

        /**
         * Set parameters for a given attribute
         */
        void setAttributeParameter(const std::string& name, bool savable, bool updateDistant)
        {
            auto attr = _attribFunctions.find(name);
            if (attr != _attribFunctions.end())
            {
                attr->second.savable(savable);
                attr->second.doUpdateDistant(updateDistant);
            }
        }

        /**
         * Register new attributes
         */
        virtual void registerAttributes() = 0;
};

/*************/
class BufferObject : public BaseObject
{
    public:
        BufferObject() {}
        BufferObject(RootObjectWeakPtr root) : BaseObject(root) {}

        virtual ~BufferObject() {}

        /**
         * Returns true if the object has been updated
         */
        bool wasUpdated() const {return _updatedBuffer | BaseObject::wasUpdated();}

        /**
         * Set the updated buffer flag to false.
         */
        void setNotUpdated() {BaseObject::setNotUpdated(); _updatedBuffer = false;}

        /**
         * Update the BufferObject from a serialized representation
         * The second definition updates from the inner serialized object
         */
        virtual bool deserialize(const std::shared_ptr<SerializedObject>& obj) = 0;
        bool deserialize()
        {
            if (!_newSerializedObject)
                return false;

            bool returnValue = deserialize(_serializedObject);
            _newSerializedObject = false;

            return returnValue;
        }

        /**
         * Get the name of the distant buffer object, for those which have a different name
         * between World and Scene (happens with Queues)
         */
        virtual std::string getDistantName() const {return _name;}

        /**
         * Get the timestamp for the current buffer object
         */
        int64_t getTimestamp() const {return _timestamp;}

        /**
         * Serialize the image
         */
        virtual std::shared_ptr<SerializedObject> serialize() const = 0;

        /**
         * Set the next serialized object to deserialize to buffer
         */
        void setSerializedObject(std::shared_ptr<SerializedObject> obj)
        {
            bool expectedAtomicValue = false;
            if (_serializedObjectWaiting.compare_exchange_strong(expectedAtomicValue, true))
            {
                _serializedObject = std::move(obj);
                _newSerializedObject = true;

                // Deserialize it right away, in a separate thread
                SThread::pool.enqueueWithoutId([this]() {
                    std::lock_guard<std::mutex> lock(_writeMutex);
                    deserialize();
                    _serializedObjectWaiting = false;
                });
            }
        }

        /**
         * Updates the timestamp of the object. Also, set the update flag to true
         */
        void updateTimestamp()
        {
            _timestamp = Timer::getTime();
            _updatedBuffer = true;
        }

    protected:
        mutable std::mutex _readMutex;
        mutable std::mutex _writeMutex;
        std::atomic_bool _serializedObjectWaiting {false};
        int64_t _timestamp;
        bool _updatedBuffer {false};

        std::shared_ptr<SerializedObject> _serializedObject;
        bool _newSerializedObject {false};
};

typedef std::shared_ptr<BufferObject> BufferObjectPtr;

/*************/
class RootObject : public BaseObject
{
    public:
        RootObject()
        {
            addAttribute("answerMessage", [&](const Values& args) {
                if (args.size() == 0 || args[0].asString() != _answerExpected)
                    return false;
                std::unique_lock<std::mutex> conditionLock(conditionMutex);
                _lastAnswerReceived = args;
                _answerCondition.notify_one();
                return true;
            });
        }

        virtual ~RootObject() {}

        /**
         * Register an object which was created elsewhere
         * If an object was the same name exists, it is replaced
         */
        void registerObject(std::shared_ptr<BaseObject> object)
        {
            if (object.get() != nullptr)
            {
                auto name = object->getName();

                std::lock_guard<std::recursive_mutex> registerLock(_objectsMutex);
                object->setSavable(false); // This object was created on the fly. Do not save it

                // We keep the previous object on the side, to prevent double free due to operator[] behavior
                auto previousObject = std::shared_ptr<BaseObject>();
                auto objectIt = _objects.find(name);
                if (objectIt != _objects.end())
                    previousObject = objectIt->second;

                _objects[name] = object;
            }
        }

        /**
         * Unregister an object which was created elsewhere, from its name,
         * sending back a shared_ptr for it
         */
        std::shared_ptr<BaseObject> unregisterObject(const std::string& name)
        {
            std::lock_guard<std::recursive_mutex> lock(_objectsMutex);

            auto objectIt = _objects.find(name);
            if (objectIt != _objects.end())
            {
                auto object = objectIt->second;
                _objects.erase(objectIt);
                return object;
            }

            return {};
        }

        /**
         * Set the attribute of the named object with the given args
         */
        bool set(const std::string& name, const std::string& attrib, const Values& args, bool async = true)
        {
            std::lock_guard<std::recursive_mutex> lock(_setMutex);

            if (name == _name || name == SPLASH_ALL_PEERS)
                return setAttribute(attrib, args);

            if (async)
            {
                addTask([=]() {
                    auto objectIt = _objects.find(name);
                    if (objectIt != _objects.end())
                        objectIt->second->setAttribute(attrib, args);
                });
            }
            else
            {
                auto objectIt = _objects.find(name);
                if (objectIt != _objects.end())
                    return objectIt->second->setAttribute(attrib, args);
                else
                    return false;
            }

            return true;
        }

        /**
         * Set an object from its serialized form
         * If non existant, it is handled by the handleSerializedObject method
         */
        void setFromSerializedObject(const std::string& name, std::shared_ptr<SerializedObject> obj)
        {
            std::lock_guard<std::recursive_mutex> lock(_setMutex);

            auto objectIt = _objects.find(name);
            if (objectIt != _objects.end())
            {
                auto object = std::dynamic_pointer_cast<BufferObject>(objectIt->second);
                if (object)
                    object->setSerializedObject(std::move(obj));
            }
            else
            {
                handleSerializedObject(name, std::move(obj));
            }
        }

    protected:
        std::shared_ptr<Link> _link;
        mutable std::recursive_mutex _objectsMutex; // Used in registration and unregistration of objects
        std::atomic_bool _objectsCurrentlyUpdated {false};
        mutable std::recursive_mutex _setMutex;
        std::unordered_map<std::string, std::shared_ptr<BaseObject>> _objects;

        Values _lastAnswerReceived {};
        std::condition_variable _answerCondition;
        std::mutex conditionMutex;
        std::mutex _answerMutex;
        std::string _answerExpected {""};
        
        // Tasks queue
        std::mutex _taskMutex;
        std::list<std::function<void()>> _taskQueue;

        virtual void handleSerializedObject(const std::string name, std::shared_ptr<SerializedObject> obj) {}

        /**
         * Add a new task to the queue
         */
        void addTask(const std::function<void()>& task)
        {
            std::lock_guard<std::mutex> lock(_taskMutex);
            _taskQueue.push_back(task);
        }

        /**
         * Send a message to the target specified by its name
         */
        void sendMessage(std::string name, std::string attribute, const Values& message = {})
        {
            _link->sendMessage(name, attribute, message);
        }

        /**
         * Send a message to the target specified by its name, and wait for an answer
         * Can specify a timeout for the answer, in microseconds
         */
        Values sendMessageWithAnswer(std::string name, std::string attribute, const Values& message = {}, const unsigned long long timeout = 0ull)
        {
            if (_link == nullptr)
                return {};

            std::lock_guard<std::mutex> lock(_answerMutex);
            _answerExpected = attribute;

            std::unique_lock<std::mutex> conditionLock(conditionMutex);
            _link->sendMessage(name, attribute, message);

            auto cvStatus = std::cv_status::no_timeout;
            if (timeout == 0ull)
                _answerCondition.wait(conditionLock);
            else
                cvStatus = _answerCondition.wait_for(conditionLock, std::chrono::microseconds(timeout));

            _answerExpected = "";

            if (std::cv_status::no_timeout == cvStatus)
                return _lastAnswerReceived;
            else
                return {};
        }
};

typedef std::shared_ptr<RootObject> RootObjectPtr;

} // end of namespace

#endif // SPLASH_BASETYPES_H
