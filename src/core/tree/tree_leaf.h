/*
 * Copyright (C) 2018 Emmanuel Durand
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
 * @tree_leaf.h
 * The Leaf class, hanging on tree branches
 */

#ifndef SPLASH_TREE_LEAF_H
#define SPLASH_TREE_LEAF_H

#include <chrono>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "./config.h"

#include "./core/value.h"

namespace Splash
{

namespace Tree
{

class Branch;
class Root;

/*************/
/**
 * Tree::Leaf class, which holds a single Value as well as a timestamp
 * Updates to a Leaf's value can trigger some callbacks.
 */
class Leaf
{
    friend Branch;
    friend Root;

  public:
    using UpdateCallback = std::function<void(Value, std::chrono::system_clock::time_point)>;
    using UpdateCallbackID = int;

  public:
    explicit Leaf(const std::string& name, Value value = {}, Branch* branch = nullptr);

    /**
     * Compare the equality of two leaves
     * \param rhs Other leaf to compare to
     * \return Return true if leaves are identical
     */
    bool operator==(const Leaf& rhs) const;
    bool operator!=(const Leaf& rhs) const { return !operator==(rhs); }

    /**
     * Add a callback
     * \param callback Callback to add
     * \return Return an ID number for the callback
     */
    UpdateCallbackID addCallback(const UpdateCallback& callback);

    /**
     * Remove a callback given its ID
     * \param id Callback ID
     * \return Return true if the callback has been successfully removed
     */
    bool removeCallback(int id);

    /**
     * Get the leaf name
     * \return Return the leaf name
     */
    std::string getName() const { return _name; }

    /**
     * Get the full path for this leaf
     * \return Return the full path
     */
    std::string getPath() const;

    /**
     * Return a string describing the leaf
     * \param indent Indent for this leaf
     * \return Return the leaf as a string
     */
    std::string print(int indent) const;

    /**
     * Set the leaf value, if the timestamp is newer than the stored one
     * \param value Value to set
     * \param timestamp Timestamp to force for this value
     * \return Return true if the value has been set, i.e. the timestamp is newer than the stored one
     */
    bool set(Value value, std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now());

    /**
     * Set leaf name
     * \param name Leaf name
     */
    void setName(const std::string& name) { _name = name; }

    /**
     * Set this leaf parent
     * \param parent Parent branch
     */
    void setParent(Branch* parent) { _parentBranch = parent; }

    /**
     * Get the leaf value, and optionaly its timestamp
     * \return Return the value
     */
    Value get() const { return _value; }
    Value get(std::chrono::system_clock::time_point& timestamp) const
    {
        timestamp = _timestamp;
        return _value;
    }

  private:
    int _currentCallbackID{0};
    std::mutex _callbackMutex;
    std::unordered_map<int, UpdateCallback> _callbacks{};
    std::chrono::system_clock::time_point _timestamp{};
    std::string _name{"leaf"};
    Value _value{};
    Branch* _parentBranch{nullptr};
};

} // namespace Tree

} // namespace Splash

#endif // SPLASH_TREE_LEAF_H
