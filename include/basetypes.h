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

#include "./coretypes.h"
#include "./link.h"
#include "./log.h"
#include "./timer.h"

namespace Splash
{

/*************/
//! AttributeFunctor class, used to add attributes to class through setter and getter functions.
struct AttributeFunctor
{
    public:
        enum class Sync
        {
            no_sync,
            force_sync,
            force_async
        };

        /**
         * \brief Default constructor.
         */
        AttributeFunctor() {};

        /**
         * \brief Constructor.
         * \param name Name of the attribute.
         * \param setFunc Setter function.
         * \param types Vector of char defining the parameters types the setter function expects.
         */
        AttributeFunctor(const std::string& name,
                         std::function<bool(const Values&)> setFunc,
                         const std::vector<char>& types = {})
        {
            _name = name;
            _setFunc = setFunc;
            _getFunc = std::function<const Values()>();
            _defaultSetAndGet = false;
            _valuesTypes = types;
        }
        

        /**
         * \brief Constructor.
         * \param name Name of the attribute.
         * \param setFunc Setter function.
         * \param getFunc Getter function.
         * \param types Vector of char defining the parameters types the setter function expects.
         */
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

        /**
         * \brief Parenthesis operator which calls the setter function if defined, otherwise calls a default setter function which only stores the arguments if the have the right type.
         * \param args Arguments as a queue of Value.
         * \return Returns true if the set did occur.
         */
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

            // Check for arguments correctness.
            // Some attributes may have an unlimited number of arguments, so we do not test for equality.
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

        /**
         * \brief Parenthesis operator which calls the getter function if defined, otherwise simply returns the stored values.
         * \return Returns the stored values.
         */
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

        /**
         * \brief Tells whether the setter and getters are the default ones or not.
         * \return Returns true if the setter and getter are the default ones.
         */
        bool isDefault() const
        {
            return _defaultSetAndGet;
        }

        /**
         * \brief Ask whether to update the Scene object (if this attribute is hosted by a World object).
         * \return Returns true if the World should update this attribute in the distant Scene object.
         */
        bool doUpdateDistant() const {return _doUpdateDistant;}

        /**
         * \brief Set whether to update the Scene object (if this attribute is hosted by a World object).
         * \return Returns true if the World should update this attribute in the distant Scene object.
         */
        void doUpdateDistant(bool update) {_doUpdateDistant = update;}

        /**
         * \brief Get the types of the wanted arguments.
         * \return Returns the expected types in a Values.
         */
        Values getArgsTypes() const
        {
            Values types {};
            for (const auto& type : _valuesTypes)
                types.push_back(Value(std::string(&type, 1)));
            return types;
        }

        /**
         * \brief Ask whether the attribute is locked.
         * \return Returns true if the attribute is locked.
         */
        bool isLocked() const {return _isLocked;}

        /**
         * \brief Lock the attribute to the given value.
         * \param v The value to set the attribute to. If empty, uses the stored value.
         * \return Returns true if the value could be locked.
         */
        bool lock(Values v = {})
        {
            if (v.size() != 0)
                if (!operator()(v))
                    return false;

            _isLocked = true;
            return true;
        }

        /**
         * \brief Unlock the attribute.
         */
        void unlock() {_isLocked = false;}

        /**
         * \brief Ask whether the attribute should be saved.
         * \return Returns true if the attribute should be saved.
         */
        bool savable() const {return _savable;}

        /**
         * \brief Set whether the attribute should be saved.
         * \param save If true, the attribute will be save.
         */
        void savable(bool save) {_savable = save;}

        /**
         * \brief Set the description.
         * \param desc Description.
         */
        void setDescription(const std::string& desc) {_description = desc;}

        /**
         * \brief Get the description.
         * \return Returns the description.
         */
        std::string getDescription() const {return _description;}

        /**
         * \brief Set the name of the object holding this attribute
         * \param objectName Name of the parent object
         */
        void setObjectName(const std::string& objectName) {_objectName = objectName;}

        /**
         * \brief Get the synchronization method for this attribute
         * \return Return the synchronization method
         */
        Sync getSyncMethod() const {return _syncMethod;}

        /**
         * \brief Set the prefered synchronization method for this attribute
         * \param method Can be Sync::no_sync, Sync::force_sync, Sync::force_async
         */
        void setSyncMethod(const Sync& method) {_syncMethod = method;}

    private:
        mutable std::mutex _defaultFuncMutex {};
        std::function<bool(const Values&)> _setFunc {};
        std::function<const Values()> _getFunc {};

        std::string _objectName; // Name of the object holding this attribute
        std::string _name; // Name of the attribute
        std::string _description {}; // Attribute description
        Values _values; // Holds the values for the default set and get functions
        std::vector<char> _valuesTypes; // List of the types held in _values
        Sync _syncMethod {Sync::no_sync}; //!< Synchronization to consider while setting this attribute

        bool _isLocked {false};

        bool _defaultSetAndGet {true};
        bool _doUpdateDistant {false}; // True if the World should send this attr values to Scenes
        bool _savable {true}; // True if this attribute should be saved
};

class BaseObject;
class RootObject;

/*************/
//! BaseObject class, which is the base class for all of classes
class BaseObject
{
    public:
        /**
         * \brief Constructor.
         */
        BaseObject()
        {
            init();
        }

        /**
         * \brief Constructor.
         * \param root Specify the root object.
         */
        BaseObject(std::weak_ptr<RootObject> root)
            : _root(root)
        {
            init();
        }

        /**
         * \brief Destructor.
         */
        virtual ~BaseObject() {}

        /**
         * \brief Safe bool idiom.
         */
        virtual explicit operator bool() const
        {
            return true;
        }

        /**
         * \brief Access the attributes through operator[].
         * \param attr Name of the attribute.
         * \return Returns a reference to the attribute.
         */
        AttributeFunctor& operator[](const std::string& attr)
        {
            auto attribFunction = _attribFunctions.find(attr);
            return attribFunction->second;
        }

        /**
         * \brief Get the real type of this BaseObject, as a std::string.
         * \return Returns the type.
         */
        inline std::string getType() const {return _type;}

        /**
         * \brief Set the ID of the object.
         * \param id ID of the object.
         */
        inline void setId(unsigned long id) {_id = id;}

        /**
         * \brief Get the ID of the object.
         * \return Returns the ID of the object.
         */
        inline unsigned long getId() const {return _id;}

        /**
         * \brief Set the name of the object.
         * \param name name of the object.
         */
        inline virtual std::string setName(const std::string& name)
        {
            _name = name;
            return _name;
        }

        /**
         * \brief Get the name of the object.
         * \return Returns the name of the object.
         */
        inline std::string getName() const {return _name;}

        /**
         * \brief Set the remote type of the object. This implies that this object gets data streamed from a World object
         * \param type Remote type
         */
        inline void setRemoteType(std::string type)
        {
            _remoteType = type;
            _isConnectedToRemote = true;
        }

        /**
         * \brief Get the remote type of the object.
         * \return Returns the remote type.
         */
        inline std::string getRemoteType() const {return _remoteType;}

        /**
         * \brief Try to link / unlink the given BaseObject to this
         * \param obj Object to link to
         * \return Returns true if the linking succeeded
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
         * \brief Unlink a given object
         * \param obj Object to unlink from
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
         * \brief Return a vector of the linked objects
         * \return Returns a vector of the linked objects
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
         * \brief Set the specified attribute
         * \param attrib Attribute name
         * \param args Values object which holds attribute values
         * \return Returns true if the parameter exists and was set
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
         * \brief Get the specified attribute
         * \param attrib Attribute name
         * \param args Values object which will hold the attribute values
         * \param includeDistant Return true even if the attribute is distant
         * \param includeNonSavable Return true even if the attribute is not savable
         * \return Return true if the parameter exists
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
         * \brief Get all the savable attributes as a map
         * \param includeDistant Also include the distant attributes
         * \return Return the map of all the attributes
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
         * \brief Get the map of the attributes which should be updated from World to Scene
         * \brief This is the case when the distant object is different from the World one
         * \return Returns a map of the distant attributes
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
         * \brief Get the savability for this object
         * \return Returns true if the object should be saved
         */
        inline bool getSavable() {return _savable;}

        /**
         * \brief Check whether the object's buffer was updated and needs to be re-rendered
         * \return Returns true if the object was updated
         */
        inline virtual bool wasUpdated() const {return _updatedParams;}

        /**
         * \brief Reset the "was updated" status, if needed
         */
        inline virtual void setNotUpdated() {_updatedParams = false;}

        /**
         * \brief Set the object savability
         * \param savable Desired savability
         */
        inline virtual void setSavable(bool savable) {_savable = savable;}
       
        /**
         * \brief Update the content of the object
         */
        virtual void update() {}

        /**
         * \brief Converts a Value as a Json object
         * \param values Value to convert
         * \return Returns a Json object
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

        /**
         * \brief Get the object's configuration as a Json object
         * \return Returns a Json object
         */
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
         * \brief Get the description for the given attribute, if it exists
         * \param name Name of the attribute
         * \return Returns the description for the attribute
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
         * \brief Get a Values holding the description of all of this object's attributes
         * \return Returns all the descriptions as a Values
         */
        Values getAttributesDescriptions()
        {
            Values descriptions;
            for (const auto& attr : _attribFunctions)
                descriptions.push_back(Values({attr.first, attr.second.getDescription(), attr.second.getArgsTypes()}));
            return descriptions;
        }

        /**
         * \brief Get the attribute synchronization method
         * \param name Attribute name
         * \return Return the synchronization method
         */
        AttributeFunctor::Sync getAttributeSyncMethod(const std::string& name)
        {
            auto attr = _attribFunctions.find(name);
            if (attr != _attribFunctions.end())
                return attr->second.getSyncMethod();
            else
                return AttributeFunctor::Sync::no_sync;
        }

    public:
        bool _savable {true}; //!< True if the object should be saved

    protected:
        unsigned long _id {0}; //!< Internal ID of the object
        std::string _type {"baseobject"}; //!< Internal type
        std::string _remoteType {""}; //!< When the object root is a Scene, this is the type of the corresponding object in the World
        std::string _name {""}; //!< Object name

        bool _isConnectedToRemote {false}; //!< True if the object gets data from a World object
        std::string _configFilePath {""}; //!< Configuration path

        std::weak_ptr<RootObject> _root; //!< Root object, Scene or World
        std::vector<std::weak_ptr<BaseObject>> _linkedObjects; //!< Children of this object

        std::unordered_map<std::string, AttributeFunctor> _attribFunctions; //!< Map of all attributes
        bool _updatedParams {true}; //!< True if the parameters have been updated and the object needs to reflect these changes

        /**
         * \brief Initialize some generic attributes
         */
        void init()
        {
            addAttribute("configFilePath", [&](const Values& args) {
                _configFilePath = args[0].asString();
                return true;
            }, {'s'});

            addAttribute("setName", [&](const Values& args) {
                setName(args[0].asString());
                return true;
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
         * \brief Add a new attribute to this object
         * \param name Attribute name
         * \param set Set function
         * \param types Vector of char holding the expected parameters for the set function
         * \return Return a reference to the created attribute
         */
        AttributeFunctor& addAttribute(const std::string& name, std::function<bool(const Values&)> set, const std::vector<char> types = {})
        {
            _attribFunctions[name] = AttributeFunctor(name, set, types);
            _attribFunctions[name].setObjectName(_type);
            return _attribFunctions[name];
        }


        /**
         * \brief Add a new attribute to this object
         * \param name Attribute name
         * \param set Set function
         * \param get Get function
         * \param types Vector of char holding the expected parameters for the set function
         * \return Return a reference to the created attribute
         */
        AttributeFunctor& addAttribute(const std::string& name, std::function<bool(const Values&)> set, std::function<const Values()> get, const std::vector<char>& types = {})
        {
            _attribFunctions[name] = AttributeFunctor(name, set, get, types);
            _attribFunctions[name].setObjectName(_type);
            return _attribFunctions[name];
        }

        /**
         * \brief Set and the description for the given attribute, if it exists
         * \param name Attribute name
         * \param description Attribute description
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
         * \brief Set attribute synchronization method
         * \param Method Synchronization method, can be any of the AttributeFunctor::Sync values
         */
        void setAttributeSyncMethod(const std::string& name, const AttributeFunctor::Sync& method)
        {
            auto attr = _attribFunctions.find(name);
            if (attr != _attribFunctions.end())
                attr->second.setSyncMethod(method);
        }

        /**
         * \brief Remove the specified attribute
         * \param name Attribute name
         */
        void removeAttribute(const std::string& name)
        {
            auto attr = _attribFunctions.find(name);
            if (attr != _attribFunctions.end())
                _attribFunctions.erase(attr);
        }

        /**
         * \brief Set additional parameters for a given attribute
         * \param name Attribute name
         * \param savable Savability
         * \param updateDistant If true and the object has a World as root, updates the attribute of the corresponding Scene object
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
};

/*************/
//! Base class for buffer objects, which are updated from outside sources. Typically, videos or live meshes
class BufferObject : public BaseObject
{
    public:
        /**
         * \brief Constructor
         */
        BufferObject() {}

        /**
         * \brief Constructor
         * \param root Root object
         */
        BufferObject(std::weak_ptr<RootObject> root) : BaseObject(root) {}

        /**
         * \brief Destructor
         */
        virtual ~BufferObject() {}

        /**
         * \brief Check whether the object has been updated
         * \return Return true if the object has been updated
         */
        bool wasUpdated() const {return _updatedBuffer | BaseObject::wasUpdated();}

        /**
         * \brief Set the updated buffer flag to false.
         */
        void setNotUpdated() {BaseObject::setNotUpdated(); _updatedBuffer = false;}

        /**
         * \brief Update the BufferObject from a serialized representation.
         * \param obj Serialized object to use as source
         * \return Return true if everything went well
         */
        virtual bool deserialize(const std::shared_ptr<SerializedObject>& obj) = 0;

        /**
         * \brief Update the BufferObject from the inner serialized object, set with setSerializedObject
         * \return Return true if everything went well
         */
        bool deserialize()
        {
            if (!_newSerializedObject)
                return false;

            bool returnValue = deserialize(_serializedObject);
            _newSerializedObject = false;

            return returnValue;
        }

        /**
         * \brief Get the name of the distant buffer object, for those which have a different name between World and Scene (happens with Queues)
         * \return Return the distant name
         */
        virtual std::string getDistantName() const {return _name;}

        /**
         * \brief Get the timestamp for the current buffer object
         * \return Return the timestamp
         */
        int64_t getTimestamp() const {return _timestamp;}

        /**
         * \brief Serialize the object
         * \return Return a serialized representation of the object
         */
        virtual std::shared_ptr<SerializedObject> serialize() const = 0;

        /**
         * \brief Set the next serialized object to deserialize to buffer
         * \param obj Serialized object
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
         * \brief Updates the timestamp of the object. Also, set the update flag to true.
         */
        void updateTimestamp()
        {
            _timestamp = Timer::getTime();
            _updatedBuffer = true;
        }

    protected:
        mutable std::mutex _readMutex; //!< Read mutex locked when the object is read from
        mutable std::mutex _writeMutex; //!< Write mutex locked when the object is written to
        std::atomic_bool _serializedObjectWaiting {false}; //!< True if a serialized object has been set and waits for processing
        int64_t _timestamp {0}; //!< Timestamp
        bool _updatedBuffer {false}; //!< True if the BufferObject has been updated

        std::shared_ptr<SerializedObject> _serializedObject; //!< Internal buffer object
        bool _newSerializedObject {false}; //!< Set to true during serialized object processing
};

/*************/
//! Base class for root objects: World and Scene
class RootObject : public BaseObject
{
    friend BaseObject; //!< Base objects can access protected members, typically _objects

    public:
        /**
         * \brief Constructor
         */
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

        /**
         * \brief Destructor
         */
        virtual ~RootObject() {}

        /**
         * \brief Register an object which was created elsewhere. If an object was the same name exists, it is replaced.
         * \param object Object to register
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
         * \brief Unregister an object which was created elsewhere, from its name, sending back a shared_ptr for it.
         * \param name Object name
         * \return Return a shared pointer to the unregistered object
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
         * \brief Set the attribute of the named object with the given args
         * \param name Object name
         * \param attrib Attribute name
         * \param args Value to set the attribute to
         * \param async Set to true for the attribute to be set asynchronously
         * \return Return true if all went well
         */
        bool set(const std::string& name, const std::string& attrib, const Values& args, bool async = true)
        {
            std::lock_guard<std::recursive_mutex> lock(_setMutex);

            if (name == _name || name == SPLASH_ALL_PEERS)
                return setAttribute(attrib, args);

            auto objectIt = _objects.find(name);
            if (objectIt != _objects.end() && objectIt->second->getAttributeSyncMethod(attrib) == AttributeFunctor::Sync::force_sync)
                async = false;

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
                if (objectIt != _objects.end())
                    return objectIt->second->setAttribute(attrib, args);
                else
                    return false;
            }

            return true;
        }

        /**
         * \brief Set an object from its serialized form. If non existant, it is handled by the handleSerializedObject method.
         * \param name Object name
         * \param obj Serialized object
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

        /**
         * \brief Send the given serialized buffer through the link
         * \param name Destination BufferObject name
         * \param buffer Serialized buffer
         */
        void sendBuffer(const std::string& name, const std::shared_ptr<SerializedObject> buffer)
        {
            _link->sendBuffer(name, buffer);
        }

    protected:
        std::shared_ptr<Link> _link; //!< Link object for communicatin between World and Scene
        mutable std::recursive_mutex _objectsMutex; //!< Used in registration and unregistration of objects
        std::atomic_bool _objectsCurrentlyUpdated {false}; //!< Prevents modification of objects from multiple places at the same time
        mutable std::recursive_mutex _setMutex; //!< Attributes mutex
        std::unordered_map<std::string, std::shared_ptr<BaseObject>> _objects; //!< Map of all the objects

        Values _lastAnswerReceived {}; //!< Holds the last answer received through the link
        std::condition_variable _answerCondition; 
        std::mutex conditionMutex;
        std::mutex _answerMutex;
        std::string _answerExpected {""};
        
        // Tasks queue
        std::mutex _taskMutex;
        std::list<std::function<void()>> _taskQueue;

        /**
         * \brief Method to process a serialized object
         * \param name Object name to receive the serialized object
         * \param obj Serialized object
         */
        virtual void handleSerializedObject(const std::string name, std::shared_ptr<SerializedObject> obj) {}

        /**
         * \brief Add a new task to the queue
         * \param task Task function
         */
        void addTask(const std::function<void()>& task)
        {
            std::lock_guard<std::mutex> lock(_taskMutex);
            _taskQueue.push_back(task);
        }

        /**
         * \brief Send a message to another root object
         * \param name Root object name
         * \param attribute Attribute name
         * \param message Message
         */
        void sendMessage(std::string name, std::string attribute, const Values& message = {})
        {
            _link->sendMessage(name, attribute, message);
        }

        /**
         * \brief Send a message to another root object, and wait for an answer. Can specify a timeout for the answer, in microseconds.
         * \param name Root object name
         * \param attribute Attribute name
         * \param message Message
         * \param timeout Timeout in microseconds
         * \return Return the answer received (or an empty Values)
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

} // end of namespace

#endif // SPLASH_BASETYPES_H
