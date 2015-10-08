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

#include <condition_variable>
#include <map>
#include <unordered_map>
#include <json/reader.h>

#include "coretypes.h"
#include "link.h"

namespace Splash
{

/*************/
struct AttributeFunctor
{
    public:
        AttributeFunctor() {};

        AttributeFunctor(std::function<bool(const Values&)> setFunc)
        {
            _setFunc = setFunc;
            _getFunc = std::function<const Values()>();
            _defaultSetAndGet = false;
        }
        
        AttributeFunctor(std::function<bool(const Values&)> setFunc,
                            std::function<const Values()> getFunc)
        {
            _setFunc = setFunc;
            _getFunc = getFunc;
            _defaultSetAndGet = false;
        }

        AttributeFunctor(const AttributeFunctor&) = delete;
        AttributeFunctor(AttributeFunctor&&) = default;
        AttributeFunctor& operator=(const AttributeFunctor&) = delete;
        AttributeFunctor& operator=(AttributeFunctor&&) = default;

        bool operator()(const Values& args)
        {
            if (!_setFunc && _defaultSetAndGet)
            {
                _values = args;
                return true;
            }
            else if (!_setFunc)
                return false;
            return _setFunc(std::forward<const Values&>(args));
        }

        Values operator()() const
        {
            if (!_getFunc && _defaultSetAndGet)
                return _values;
            else if (!_getFunc)
                return Values();
            return _getFunc();
        }

        bool isDefault() const
        {
            return _defaultSetAndGet;
        }

        bool doUpdateDistant() const {return _doUpdateDistant;}
        void doUpdateDistant(bool update) {_doUpdateDistant = update;}

    private:
        std::function<bool(const Values&)> _setFunc {};
        std::function<const Values()> _getFunc {};

        bool _defaultSetAndGet {true};
        Values _values; // Holds the values for the default set and get functions

        bool _doUpdateDistant {false}; // True if the World should send this attr values to Scenes
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

        std::string getType() const {return _type;}

        /**
         * Set and get the id of the object
         */
        unsigned long getId() const {return _id;}
        void setId(unsigned long id) {_id = id;}

        /**
         * Set and get the name of the object
         */
        std::string getName() const {return _name;}
        void setName(std::string name) {_name = name;}

        /**
         * Set and get the remote type of the object
         */
        std::string getRemoteType() const {return _remoteType;}
        void setRemoteType(std::string type) {_remoteType = type;}

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
        virtual bool unlinkFrom(std::shared_ptr<BaseObject> obj)
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
            {
                _linkedObjects.erase(objectIt);
                return true;
            }
            return false;
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
                auto result = _attribFunctions.emplace(std::make_pair(attrib, AttributeFunctor()));
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
         */
        bool getAttribute(const std::string& attrib, Values& args, bool includeDistant = false) const
        {
            auto attribFunction = _attribFunctions.find(attrib);
            if (attribFunction == _attribFunctions.end())
                return false;

            args = attribFunction->second();

            if (attribFunction->second.isDefault() && !includeDistant)
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
                if (getAttribute(attr.first, values, includeDistant) == false || values.size() == 0)
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
                if (getAttribute(attr.first, values) == false || values.size() == 0)
                    continue;

                attribs[attr.first] = values;
            }

            return attribs;
        }

        /**
         * Check whether the objects needs to be updated
         */
        virtual bool wasUpdated() const {return _updatedParams;}

        /**
         * Reset the "was updated" status, if needed
         */
        virtual void setNotUpdated() {_updatedParams = false;}
        
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

    // Pubic attributes
    public:
        bool _savable {true};

    protected:
        unsigned long _id;
        std::string _type {"baseobject"};
        std::string _remoteType {""};
        std::string _name {""};

        std::string _configFilePath {""}; // All objects know about their location

        RootObjectWeakPtr _root;
        std::vector<std::weak_ptr<BaseObject>> _linkedObjects;

        std::unordered_map<std::string, AttributeFunctor> _attribFunctions;
        bool _updatedParams {true};

        // Initialize generic attributes
        void init()
        {
            _attribFunctions["configFilePath"] = AttributeFunctor([&](const Values& args) {
                if (args.size() == 0)
                    return false;
                _configFilePath = args[0].asString();
                return true;
            });

            _attribFunctions["setName"] = AttributeFunctor([&](const Values& args) {
                if (args.size() == 0)
                    return false;
                _name = args[0].asString();
                return true;
            });
        }

        /**
         * Register new functors to modify attributes
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
        virtual bool deserialize(std::unique_ptr<SerializedObject> obj) = 0;
        bool deserialize()
        {
            if (_newSerializedObject == false)
                return true;

            bool _returnValue = deserialize(std::move(_serializedObject));
            _newSerializedObject = false;

            return _returnValue;
        }

        /**
         * Serialize the image
         */
        virtual std::unique_ptr<SerializedObject> serialize() const = 0;

        /**
         * Set the next serialized object to deserialize to buffer
         */
        void setSerializedObject(std::unique_ptr<SerializedObject> obj)
        {
            {
                std::unique_lock<std::mutex> lock(_writeMutex);
                _serializedObject = move(obj);
                _newSerializedObject = true;
            }

            // Deserialize it right away, in a separate thread
            SThread::pool.enqueueWithoutId([&]() {
                deserialize();
            });
        }

        /**
         * Updates the timestamp of the object. Also, set the update flag to true
         */
        void updateTimestamp()
        {
            _timestamp = std::chrono::high_resolution_clock::now();
            _updatedBuffer = true;
        }

    protected:
        mutable std::mutex _readMutex;
        mutable std::mutex _writeMutex;
        std::chrono::high_resolution_clock::time_point _timestamp;
        bool _updatedBuffer {false};

        std::unique_ptr<SerializedObject> _serializedObject;
        bool _newSerializedObject {false};
};

typedef std::shared_ptr<BufferObject> BufferObjectPtr;

/*************/
class RootObject : public BaseObject
{
    public:
        RootObject()
        {
            _attribFunctions["answerMessage"] = AttributeFunctor([&](const Values& args) {
                if (args.size() == 0 || args[0].asString() != _answerExpected)
                    return false;
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
                object->_savable = false; // This object was created on the fly. Do not save it
                _objects[object->getName()] = object;
            }
        }

        /**
         * Unregister an object which was created elsewhere, from its name,
         * sending back a shared_ptr for it
         */
        std::shared_ptr<BaseObject> unregisterObject(std::string name)
        {
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
        bool set(std::string name, std::string attrib, const Values& args)
        {
            std::unique_lock<std::mutex> lock(_setMutex);
            if (name == _name || name == SPLASH_ALL_PAIRS)
                return setAttribute(attrib, args);
            else if (_objects.find(name) != _objects.end())
                return _objects[name]->setAttribute(attrib, args);
            else
                return false;
        }

        /**
         * Set an object from its serialized form
         * If non existant, it is handled by the handleSerializedObject method
         */
        void setFromSerializedObject(const std::string name, std::unique_ptr<SerializedObject> obj)
        {
            std::unique_lock<std::mutex> lock(_setMutex);
            auto objectIt = _objects.find(name);
            if (objectIt != _objects.end() && std::dynamic_pointer_cast<BufferObject>(objectIt->second).get() != nullptr)
                std::dynamic_pointer_cast<BufferObject>(objectIt->second)->setSerializedObject(std::move(obj));
            else
                handleSerializedObject(name, std::move(obj));
        }

    protected:
        std::shared_ptr<Link> _link;
        mutable std::mutex _setMutex;
        std::map<std::string, std::shared_ptr<BaseObject>> _objects;

        Values _lastAnswerReceived {};
        std::condition_variable _answerCondition;
        std::mutex _answerMutex;
        std::string _answerExpected {""};

        virtual void handleSerializedObject(const std::string name, std::unique_ptr<SerializedObject> obj) {}

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

            std::unique_lock<std::mutex> lock(_answerMutex);
            _answerExpected = attribute;
            _link->sendMessage(name, attribute, message);

            std::mutex conditionMutex;
            std::unique_lock<std::mutex> conditionLock(conditionMutex);

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
