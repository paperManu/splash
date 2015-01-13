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
 * blobserver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with blobserver.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @basetypes.h
 * Base types, from which more complex classes derive
 */

#ifndef SPLASH_BASETYPES_H
#define SPLASH_BASETYPES_H

#include <condition_variable>

#include "coretypes.h"
#include "link.h"

namespace Splash
{

/*************/
struct AttributeFunctor
{
    public:
        AttributeFunctor() {}
        AttributeFunctor(std::function<bool(Values)> setFunc) {_setFunc = setFunc;}
        AttributeFunctor(std::function<bool(Values)> setFunc,
                            std::function<Values()> getFunc) {_setFunc = setFunc; _getFunc = getFunc;}

        bool operator()(Values args)
        {
            if (!_setFunc)
                return false;
            return _setFunc(args);
        }
        Values operator()()
        {
            if (!_getFunc)
                return Values();
            return _getFunc();
        }

    private:
        std::function<bool(Values)> _setFunc;
        std::function<Values()> _getFunc;
};

class BaseObject;
typedef std::shared_ptr<BaseObject> BaseObjectPtr;
class RootObject;
typedef std::weak_ptr<RootObject> RootObjectWeakPtr;

/*************/
class BaseObject
{
    public:
        BaseObject() {}
        BaseObject(RootObjectWeakPtr root) {_root = root;}
        virtual ~BaseObject() {}

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
        virtual bool linkTo(BaseObjectPtr obj)
        {
            if (std::find(_linkedObjects.begin(), _linkedObjects.end(), obj) == _linkedObjects.end())
            {
                _linkedObjects.push_back(obj);
                return true;
            }
            return false;
        }

        virtual bool unlinkFrom(BaseObjectPtr obj)
        {
            auto objIterator = std::find(_linkedObjects.begin(), _linkedObjects.end(), obj);
            if (objIterator != _linkedObjects.end())
            {
                _linkedObjects.erase(objIterator);
                return true;
            }
            return false;
        }

        /**
         * Set the specified attribute
         */
        bool setAttribute(std::string attrib, Values args)
        {
            if (_attribFunctions.find(attrib) == _attribFunctions.end())
                return false;
            _updatedParams = true;
            return _attribFunctions[attrib](args);
        }

        /**
         * Get the specified attribute
         */
        bool getAttribute(std::string attrib, Values& args)
        {
            if (_attribFunctions.find(attrib) == _attribFunctions.end())
                return false;
            args = _attribFunctions[attrib]();
            return true;
        }

        /**
         * Get all the attributes as a map
         */
        std::map<std::string, Values> getAttributes()
        {
            std::map<std::string, Values> attribs;
            for (auto& attr : _attribFunctions)
            {
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
        virtual bool wasUpdated() {return _updatedParams;}

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
        Json::Value getValuesAsJson(Values values)
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

        Json::Value getConfigurationAsJson()
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

        RootObjectWeakPtr _root;
        std::vector<BaseObjectPtr> _linkedObjects;

        std::map<std::string, AttributeFunctor> _attribFunctions;
        bool _updatedParams {true};

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
        bool wasUpdated() {return _updatedBuffer | BaseObject::wasUpdated();}

        /**
         * Set the updated buffer flag to false.
         */
        void setNotUpdated() {BaseObject::setNotUpdated(); _updatedBuffer = false;}

        /**
         * Update the Image from a serialized representation
         * The second definition updates from the inner serialized object
         */
        virtual bool deserialize(const SerializedObjectPtr obj) = 0;
        bool deserialize()
        {
            if (_newSerializedObject == false)
                return true;

            bool _returnValue = deserialize(_serializedObject);
            _newSerializedObject = false;

            return _returnValue;
        }

        /**
         * Serialize the image
         */
        virtual SerializedObjectPtr serialize() const = 0;

        /**
         * Set the next serialized object to deserialize to buffer
         */
        void setSerializedObject(SerializedObjectPtr obj)
        {
            {
                std::lock_guard<std::mutex> lock(_writeMutex);
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

        SerializedObjectPtr _serializedObject;
        bool _newSerializedObject {false};
};

typedef std::shared_ptr<BufferObject> BufferObjectPtr;

/*************/
class RootObject : public BaseObject
{
    public:
        RootObject()
        {
            _attribFunctions["answerMessage"] = AttributeFunctor([&](Values args) {
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
        void registerObject(BaseObjectPtr object)
        {
            if (object.get() != nullptr)
            {
                object->_savable = false; // This object was created on the fly. Do not save it
                _objects[object->getName()] = object;
            }
        }

        /**
         * Set the attribute of the named object with the given args
         */
        bool set(std::string name, std::string attrib, Values args)
        {
            std::lock_guard<std::mutex> lock(_setMutex);
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
        void setFromSerializedObject(const std::string name, const SerializedObjectPtr obj)
        {
            std::lock_guard<std::mutex> lock(_setMutex);
            if (_objects.find(name) != _objects.end() && std::dynamic_pointer_cast<BufferObject>(_objects[name]).get() != nullptr)
                std::dynamic_pointer_cast<BufferObject>(_objects[name])->setSerializedObject(obj);
            else
                handleSerializedObject(name, obj);
        }

    protected:
        std::shared_ptr<Link> _link;
        mutable std::mutex _setMutex;
        std::map<std::string, BaseObjectPtr> _objects;

        Values _lastAnswerReceived {};
        std::condition_variable _answerCondition;
        std::mutex _answerMutex;
        std::string _answerExpected {""};

        virtual void handleSerializedObject(const std::string name, const SerializedObjectPtr obj) {}

        /**
         * Send a message to the target specified by its name
         */
        void sendMessage(std::string name, std::string attribute, const Values message = {})
        {
            _link->sendMessage(name, attribute, message);
        }

        /**
         * Send a message to the target specified by its name, and wait for an answer
         */
        Values sendMessageWithAnswer(std::string name, std::string attribute, const Values message = {})
        {
            if (_link == nullptr)
                return {};

            std::lock_guard<std::mutex> lock(_answerMutex);
            _answerExpected = attribute;
            _link->sendMessage(name, attribute, message);

            std::mutex conditionMutex;
            std::unique_lock<std::mutex> conditionLock(conditionMutex);
            _answerCondition.wait(conditionLock);

            _answerExpected = "";
            return _lastAnswerReceived;
        }
};

typedef std::shared_ptr<RootObject> RootObjectPtr;

} // end of namespace

#endif // SPLASH_BASETYPES_H
