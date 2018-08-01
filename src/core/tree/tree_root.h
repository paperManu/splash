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
 * @tree_root.h
 * The Root class, base of every trees
 */

#ifndef SPLASH_TREE_ROOT_H
#define SPLASH_TREE_ROOT_H

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

#include "./core/tree/tree_branch.h"
#include "./core/tree/tree_leaf.h"
#include "./core/value.h"
#include "./utils/uuid.h"

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
enum class Task : uint8_t
{
    AddBranch,
    AddLeaf,
    RemoveBranch,
    RemoveLeaf,
    SetLeaf
};

/**
 * A seed represents a change with given parameters applied at a given timepoint to a Tree of a given name
 * Seeds are automatically generated every time the Tree is modified,
 * and a list of all changes can be retrieved using Tree::getSeedList()
 * This allows for replicating changes from a Tree to another
 */
using Seed = std::tuple<Task, Value, std::chrono::system_clock::time_point, UUID>;

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
     * Add a seed to the queue of changes to apply to the tree.
     * \param taskType Task type
     * \param args Task arguments
     * \param timestamp Time point at which the change happened
     */
    void addSeedToQueue(Tree::Task taskType, Values args, std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now());

    /**
     * Add a list of seeds to the queue
     * \param seeds Tasks and arguments list to add to the queue
     */
    void addSeedsToQueue(const std::list<Seed>& seeds);

    /**
     * Clear the seed list
     */
    void clearSeedList();

    /**
     * Get whether an error is set
     * \return Return true if an error is set
     */
    bool hasError() const { return _hasError; }

    /**
     * Return whether the given leaf exists
     * \param path Path to the leaf
     */
    bool hasLeafAt(const std::string& path) const;

    /**
     * Get the oldest error, and resets the error flag
     * \param error Error string
     * \return Return true if there was an error
     */
    bool getError(std::string& error);

    /**
     * Get the list of branches connected to the root
     * \return Return the list of branches
     */
    std::list<std::string> getBranchList() const { return _rootBranch->getBranchList(); }

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
     * Process the seeds queue to update the tree
     * \param propagate If true, the seeds are duplicated inside the updates seed list for propagation to other trees
     * \return Return true if everything went well. Otherwise, return false and sets an error message.
     */
    bool processQueue(bool propagate = false);

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
     * Set the root's name
     * \param name Desired name
     */
    void setName(const std::string& name) { _name = name; }

    /**
     * Return a string describing the tree
     * \return Return the tree as a string
     */
    std::string print() const;

  private:
    UUID _uuid{};
    std::string _name{"root"};
    std::unique_ptr<Branch> _rootBranch{nullptr};
    std::mutex _taskMutex{};
    std::mutex _updatesMutex{};
    std::list<Seed> _taskQueue{}; //!< Queue of seeds to be applied to the tree by calling processQueue()
    std::list<Seed> _updates{};   //!< Queue of seeds generated by operations done onto the tree

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

} // namespace Tree

} // namespace Splash

#endif // SPLASH_TREE_ROOT_H
