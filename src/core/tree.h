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
 * @tree.h
 * The Tree class, responsible for synchronizing multiple parts of Splash
 * It has the particularity of generating a Seed for every change applied to it,
 * which can then be used for replicating the change into another Tree
 */

#ifndef SPLASH_TREE_H
#define SPLASH_TREE_H

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
class Leaf;

/*************/
/**
 * A Task is a change type applied to a Tree
 */
enum class Task
{
    AddBranch,
    AddLeaf,
    RemoveBranch,
    RemoveLeaf,
    SetLeaf
};

/**
 * A seed represents a change applied to a Tree at a given timepoint
 * Seeds are automatically generated every time the Tree is modified,
 * and a list of all changes can be retrieved using Tree::getSeedList()
 * This allows for replicating changes from a Tree to another
 */
using Seed = std::tuple<Task, Value, std::chrono::system_clock::time_point>;

/*************/
/**
 * Tree::Root class, which holds the main branch
 * All commands should be applied to this class directly, otherwise
 * the Seeds are not generated when changes are made
 */
class Root
{
  public:
    /**
     * Constructor
     */
    Root();

    /**
     * Check roots equality
     * \param rhs Other root to compare to
     * \return Return true if both roots are identical
     */
    bool operator==(const Root& rhs) const;
    bool operator!=(const Root& rhs) const { return !operator==(rhs); }

    /**
     * Add a branch at the given path
     * \param path Path of the branch
     * \param silent do not add this action to the update list
     * \return Return true if the branch was created successfully
     */
    bool addBranchAt(const std::string& path, bool silent = false);

    /**
     * Add a leaf at the given path
     * \param path Path of the leaf
     * \param value Value for the leaf
     * \param silent do not add this action to the update list
     * \return Return true if the leaf was created successfully
     */
    bool addLeafAt(const std::string& path, Values value = {}, bool silent = false);

    /**
     * Add a task to the queue of changes to apply to the tree.
     * \param taskType Task type
     * \param args Task arguments
     * \param timestamp Time point at which the change happened
     */
    void addTaskToQueue(Tree::Task taskType, Values args, std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now());

    /**
     * Add a list of tasks to the queue
     * \param seeds Tasks and arguments list to add to the queue
     */
    void addTasksToQueue(const std::list<Seed>& seeds);

    /**
     * Get whether an error is set
     * \return Return true if an error is set
     */
    bool hasError() const { return _hasError; }

    /**
     * Get the oldest error, and resets the error flag
     * \param error Error string
     * \return Return true if there was an error
     */
    bool getError(std::string& error);

    /**
     * Get a pointer to the branch at the given path
     * \param path Path to the branch
     * \return Return the branch, or nullptr
     */
    Branch* getBranchAt(const std::string& path) const;

    /**
     * Get a pointer to the leaf at the given path
     * \param path Path to the leaf
     * \return Return the leaf, or nullptr
     */
    Leaf* getLeafAt(const std::string& path) const;

    /**
     * Get the root name
     * \return Return the root name
     */
    std::string getName() const { return _name; }

    /**
     * Get the value held by the given leaf
     * \param path Path to the leaf
     * \param value The value of the leaf, or an empty value
     * \return Return true if the leaf was found
     */
    bool getValueForLeafAt(const std::string& path, Value& value);

    /**
     * Set the value for the leaf at the given path
     * \param path Path to the leaf
     * \param value Leaf value
     * \param timestamp Timestamp, in ms for the first definition
     * \param silent do not add this action to the update list
     * \return Return true if all went well
     */
    bool setValueForLeafAt(const std::string& path, const Values& value, int64_t timestamp = 0, bool silent = false);
    bool setValueForLeafAt(const std::string& path, const Values& value, std::chrono::system_clock::time_point timestamp, bool silent = false);

    /**
     * Get the Seeds generated while modifying the tree
     * This clears the updates queue.
     * \return Return the list of updates
     */
    std::list<Seed> getSeedList();

    /**
     * Process the queue to update the tree
     * \return Return true if everything went well. Otherwise, return false and sets an error message.
     */
    bool processQueue();

    /**
     * Remove the branch at the given path
     * \param path Path to the branch
     * \param silent do not add this action to the update list
     * \return Return true if the branch was removed successfully
     */
    bool removeBranchAt(const std::string& path, bool silent = false);

    /**
     * Remove the leaf at the given path
     * \param path Path to the leaf
     * \param silent do not add this action to the update list
     * \return Return true if the leaf was removed successfully
     */
    bool removeLeafAt(const std::string& path, bool silent = false);

    /**
     * Return a string describing the tree
     * \return Return the tree as a string
     */
    std::string print() const;

  private:
    std::string _name{"root"};
    std::unique_ptr<Branch> _rootBranch{nullptr};
    std::mutex _taskMutex{};
    std::mutex _updatesMutex{};
    std::list<Seed> _taskQueue{};
    std::list<Seed> _updates{};

    bool _hasError{false};
    std::string _errorMsg{};

    /**
     * Get a pointer to the branch at the given path
     * \param path Path as a list of strings
     * \return Return the branch, or nullptr
     */
    Branch* getBranchAt(const std::list<std::string>& path) const;

    /**
     * Get a pointer to the leaf at the given path
     * \param path Path as a list of strings
     * \return Return the leaf, or nullptr
     */
    Leaf* getLeafAt(const std::list<std::string>& path) const;

    /**
     * Extract the multiple parts of the given path
     * \param path Path
     * \return Return a list of strings describing the path
     */
    static std::list<std::string> processPath(const std::string& path);
};

/*************/
/**
 * Tree::Branch class, which can hold other Branches and Leafs
 */
class Branch
{
    friend Root;

  public:
    /**
     * Constructor
     */
    explicit Branch(const std::string& name, Branch* parent = nullptr);

    /**
     * Compare the equality of two branches
     * \param rhs Other branch to compare to
     * \return Return true if branches are identical
     */
    bool operator==(const Branch& rhs) const;
    bool operator!=(const Branch& rhs) const { return !operator==(rhs); }

  private:
    std::string _name{"branch"};
    std::unordered_map<std::string, std::unique_ptr<Branch>> _branches{};
    std::unordered_map<std::string, std::unique_ptr<Leaf>> _leaves{};
    Branch* _parentBranch{nullptr};

    /**
     * Add a new branch
     * \param branch Branch to add
     * \return Return false if a branch with the same name already exists
     */
    bool addBranch(std::unique_ptr<Branch>&& branch);

    /**
     * Add a new leaf
     * \param leaf Leaf to add
     * \return Return false if a leaf with the same name already exists
     */
    bool addLeaf(std::unique_ptr<Leaf>&& leaf);

    /**
     * Get the branch by its name
     * \param path Path to a subbranch
     * \return Return a pointer to the branch, or nullptr
     */
    Branch* getBranch(const std::string& path);

    /**
     * Get the leaf by its name
     * \param name Leaf name
     * \return Return a pointer to the leaf, or nullptr
     */
    Leaf* getLeaf(const std::string& path);

    /**
     * Get the list of leaves for this branch
     * \return Return the list of leaf names
     */
    std::list<std::string> getLeafNames() const;

    /**
     * Get the branch name
     * \return Return the name
     */
    std::string getName() const { return _name; }

    /**
     * Check whether a given subbranch exists
     * \return Return true if the subbranch exists
     */
    bool hasBranch(const std::string& name) const { return _branches.find(name) != _branches.end(); }

    /**
     * Check whether a given leaf exists
     * \return Return true if the leaf exists
     */
    bool hasLeaf(const std::string& name) const { return _leaves.find(name) != _leaves.end(); }

    /**
     * Get the full path for this branch
     * \return Return the full path
     */
    std::string getPath() const;

    /**
     * Return a string describing the branch
     * \param indent Indent for this branch
     * \return Return the tree as a string
     */
    std::string print(int indent) const;

    /**
     * Remove a branch given its name
     * \param name Branch name
     * \return Return true if the branch has been removed
     */
    bool removeBranch(std::string name);

    /**
     * Remove a leaf given its name
     * \param name Leaf name
     * \return Return true if the leaf has been removed
     */
    bool removeLeaf(std::string name);

    /**
     * Set this branch parent
     * \param parent Parent branch
     */
    void setParent(Branch* parent) { _parentBranch = parent; }
};

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
    int addCallback(const UpdateCallback& callback);

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

#endif // SPLASH_TREE_H
