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
    RenameBranch,
    RenameLeaf,
    SetLeaf
};

using CallbackID = int;

/**
 * A seed represents a change with given parameters applied at a given timepoint to a Tree of a given name
 * Seeds are automatically generated every time the Tree is modified,
 * and a list of all changes can be retrieved using Tree::getSeedList()
 * This allows for replicating changes from a Tree to another
 */
using Seed = std::tuple<Task, Value, std::chrono::system_clock::time_point, UUID>;

class Root;

/**
 * Tree Handle. The tree is locked as long as it exists
 */
class RootHandle
{
  public:
    explicit RootHandle(Tree::Root* root);
    Root* operator->() const noexcept { return _root; }

  private:
    Tree::Root* _root;
    std::unique_lock<std::recursive_mutex> _rootLock;
};

/**
 * Tree::Root class, which holds the main branch
 * All commands should be applied to this class directly, otherwise
 * the Seeds are not generated when changes are made
 */
class Root
{
    friend RootHandle;

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
     * Add the given branch at the given path. The tree acquires the ownership over the branch
     * \param path Path to the parent branch
     * \param branch Branch to add
     * \param silent do not add this action to the update list
     * \return Return true if the branch has been added, false otherwise
     */
    bool addBranchAt(const std::string& path, std::unique_ptr<Branch>&& branch, bool silent = false);

    /**
     * Add the given leaf at the given path. The tree acquires the ownership over the leaf
     * \param path Path to the parent branch
     * \param leaf Leaf to add
     * \param silent do not add this action to the update list
     * \return Return true if the leaf has been added, false otherwise
     */
    bool addLeafAt(const std::string& path, std::unique_ptr<Leaf>&& leaf, bool silent = false);

    /**
     * Add a callback to the given branch
     * \param path Path to the branch
     * \param target Targeted event for the callback
     * \param cb Callback
     * \param pending If true, the callback will be added once the branch appears
     * \return Return the ID of the callback if it has been successfully added, 0 if not, and -1 if it is pending
     */
    Branch::UpdateCallbackID addCallbackToBranchAt(const std::string& path, Branch::Task target, const Branch::UpdateCallback& cb, bool pending = false);

    /**
     * Add a callback to the given leaf
     * \param path Path to the leaf
     * \param cb Callback
     * \param pending If true, the callback will be added once the leaf appears
     * \return Return the ID of the callback if it has been successfully added, 0 if not, and -1 if it is pending
     */
    Leaf::UpdateCallbackID addCallbackToLeafAt(const std::string& path, const Leaf::UpdateCallback& cb, bool pending = false);

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
    void addSeedsToQueue(std::list<Seed> seeds);

    /**
     * Clear the seed list
     */
    void clearSeedList();

    /**
     * Clear the tree to start anew
     */
    void cutdown();

    /**
     * Add a branch at the given path
     * \param path Path of the branch
     * \param silent do not add this action to the update list
     * \return Return true if the branch was created successfully
     */
    bool createBranchAt(const std::string& path, bool silent = false);

    /**
     * Create a leaf at the given path
     * \param path Path of the leaf
     * \param value Value for the leaf
     * \param silent do not add this action to the update list
     * \return Return true if the leaf was created successfully
     */
    bool createLeafAt(const std::string& path, Values value = {}, bool silent = false);

    /**
     * Cut the branch at the given path. The tree loses ownership over it
     * \param path Path to the branch
     * \param silent do not add this action to the update list
     * \return Return a pointer to the branch, which may be nullptr if it failed
     */
    std::unique_ptr<Branch> cutBranchAt(const std::string& path, bool silent = false);

    /**
     * Cut the leaf at the given path. The tree loses ownership over it
     * \param path Path to the leaf
     * \param silent do not add this action to the update list
     * \return Return a pointer to the leaf, which may be nullptr if it failed
     */
    std::unique_ptr<Leaf> cutLeafAt(const std::string& path, bool silent = false);

    /**
     * Get the seeds for the given path
     * \param path Path to generate the seed for
     * \return Return the seeds list
     */
    std::list<Seed> getSeedsForPath(const std::string& path);

    /**
     * Get whether an error is set
     * \return Return true if an error is set
     */
    bool hasError() const { return _hasError; }

    /**
     * Return whether the given branch exists
     * \param path Path to the branch
     * \return Return true if the branch exists
     */
    bool hasBranchAt(const std::string& path) const;

    /**
     * Return whether the given leaf exists
     * \param path Path to the leaf
     * \return Return true if the leaf exists
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
     * \param path Optional branch path
     * \return Return the list of branches
     */
    std::list<std::string> getBranchList() const { return _rootBranch->getBranchList(); }
    std::list<std::string> getBranchListAt(const std::string& path) const;

    /**
     * Get the list of leaves connected to the root
     * \param path Optional branch path
     * \return Return the list of leaves
     */
    std::list<std::string> getLeafList() const { return _rootBranch->getLeafList(); }
    std::list<std::string> getLeafListAt(const std::string& path) const;

    /**
     * Get a handle over this root
     * \param Returned handle
     */
    Tree::RootHandle getHandle() { return Tree::RootHandle(this); }

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
    bool getValueForLeafAt(const std::string& path, Value& value) const;

    /**
     * Set the value for the leaf at the given path
     * \param path Path to the leaf
     * \param value Leaf value
     * \param timestamp Timestamp, in ms for the first definition
     * \param silent do not add this action to the update list
     * \return Return true if all went well
     */
    bool setValueForLeafAt(const std::string& path, const Value& value, int64_t timestamp = 0, bool silent = false);
    bool setValueForLeafAt(const std::string& path, const Value& value, std::chrono::system_clock::time_point timestamp, bool silent = false);
    // The following overloads handle conversion from Values to Value (second argument)
    bool setValueForLeafAt(const std::string& path, const Values& value, int64_t timestamp = 0, bool silent = false) { return setValueForLeafAt(path, Value(value), timestamp, silent); }
    bool setValueForLeafAt(const std::string& path, const Values& value, std::chrono::system_clock::time_point timestamp, bool silent = false)
    {
        return setValueForLeafAt(path, Value(value), timestamp, silent);
    }

    /**
     * Get the Seeds generated while modifying the tree
     * This clears the updates queue.
     * \return Return the list of updates
     */
    std::list<Seed> getSeedList();

    /**
     * Process the seeds queue to update the tree. Also register pending leaf callbacks
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
     * Remove callback from a branch given its path and ID
     * \param path Branch path
     * \param id Callback ID
     * \return Return true if the callback has been successfully removed
     */
    bool removeCallbackFromBranchAt(const std::string& path, int id);

    /**
     * Remove callback from a leaf given its path and ID
     * \param path Leaf path
     * \param id Callback ID
     * \return Return true if the callback has been successfully removed
     */
    bool removeCallbackFromLeafAt(const std::string& path, int id);

    /**
     * Rename the branch at the given path to the given name
     * \param path Path to the branch
     * \param name New name
     * \param silent do not add this action to the update list
     * \return Return true if the branch was renamed successfully
     */
    bool renameBranchAt(const std::string& path, const std::string& name, bool silent = false);

    /**
     * Rename the leaf at the given path to the given name
     * \param path Path to the leaf
     * \param name New name
     * \param silent do not add this action to the update list
     * \return Return true if the leaf was renamed successfully
     */
    bool renameLeafAt(const std::string& path, const std::string& name, bool silent = false);

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

  protected:
    mutable std::recursive_mutex _treeMutex{};

  private:
    UUID _uuid{};
    std::string _name{"root"};
    std::unique_ptr<Branch> _rootBranch{nullptr};
    mutable std::mutex _taskMutex{};
    mutable std::recursive_mutex _updatesMutex{};
    std::list<Seed> _seedQueue{}; //!< Queue of seeds to be applied to the tree by calling processQueue()
    std::list<Seed> _updates{};   //!< Queue of seeds generated by operations done onto the tree

    std::list<std::tuple<std::string, Branch::Task, Branch::UpdateCallback>> _branchCallbacksToRegister{};
    std::list<std::pair<std::string, Leaf::UpdateCallback>> _leafCallbacksToRegister{};

    bool _hasError{false};
    std::string _errorMsg{};

    /**
     * Generate a seed list to recreate the given branch
     * \param branch Branch to recreate
     * \param Return the seed list
     */
    std::list<Seed> generateSeedsForBranch(Branch* branch);

    /**
     * Generate a seed list to recreate the given leaf
     * \param branch leaf to recreate
     * \param Return the seed list
     */
    std::list<Seed> generateSeedsForLeaf(Leaf* leaf);

    /**
     * Get a pointer to the branch at the given path
     * \param path Path to the branch
     * \return Return the branch, or nullptr
     */
    Branch* getBranchAt(const std::string& path) const;

    /**
     * Get a pointer to the branch at the given path
     * \param path Path as a list of strings
     * \return Return the branch, or nullptr
     */
    Branch* getBranchAt(const std::list<std::string>& path) const;

    /**
     * Get a pointer to the leaf at the given path
     * \param path Path to the leaf
     * \return Return the leaf, or nullptr
     */
    Leaf* getLeafAt(const std::string& path) const;

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

    /**
     * Register the pending callbacks
     */
    void registerPendingCallbacks();
};

} // namespace Tree

} // namespace Splash

#endif // SPLASH_TREE_ROOT_H
