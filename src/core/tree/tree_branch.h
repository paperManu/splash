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
 * @tree_branch.h
 * The Branch class, fixed on a tree Root
 */

#ifndef SPLASH_TREE_BRANCH_H
#define SPLASH_TREE_BRANCH_H

#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <set>
#include <unordered_map>
#include <utility>

#include "./config.h"

namespace Splash
{

namespace Tree
{

class Root;
class Leaf;

/*************/
/**
 * Tree::Branch class, which can hold other Branches and Leafs
 */
class Branch
{
    friend Root;

  public:
    using UpdateCallback = std::function<void(Branch&, std::string)>;
    using UpdateCallbackID = int;


    /**
     * Targets for callbacks
     */
    enum class Task : uint8_t
    {
        AddBranch,
        AddLeaf,
        RemoveBranch,
        RemoveLeaf
    };

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

    /**
     * Add a new branch. The tree gets ownership over the branch
     * \param branch Branch to add
     * \return Return true if the branch was successfully added
     */
    bool addBranch(std::unique_ptr<Branch>&& branch);

    /**
     * Add a new leaf. The tree gets ownership over the leaf
     * \param leaf Leaf to add
     * \return Return true if the leaf was successfully added
     */
    bool addLeaf(std::unique_ptr<Leaf>&& leaf);

    /**
     * Add a callback for the given target operation
     * \param target Callback target operation
     * \param callback Callback to add
     * \return Return an ID number for the callback
     */
    UpdateCallbackID addCallback(Task target, const UpdateCallback& callback);

    /**
     * Remove a callback given its ID
     * \param id Callback ID
     * \return Return true if the callback has been removed successfully
     */
    bool removeCallback(int id);

    /**
     * Cut a branch from the current one, acquiring its ownership in the process
     * \param branch Name of the branch to cut
     * \return Return a pointer to the cut branch
     */
    std::unique_ptr<Branch> cutBranch(const std::string& branchName);

    /**
     * Cut a leaf from the current one, acquiring its ownership in the process
     * \param leaf Name of the leaf to cut
     * \return Return a pointer to the cut leaf
     */
    std::unique_ptr<Leaf> cutLeaf(const std::string& leafName);

    /**
     * Get the branch by its name
     * \param path Path to a subbranch
     * \return Return a pointer to the branch, or nullptr
     */
    Branch* getBranch(const std::string& path);

    /**
     * Get the list of branches connected to this branch
     * \return Return the list of branches
     */
    std::list<std::string> getBranchList() const;

    /**
     * Get the leaf by its name
     * \param name Leaf name
     * \return Return a pointer to the leaf, or nullptr
     */
    Leaf* getLeaf(const std::string& path);

    /**
     * Get the list of leaves connected to this branch
     * \return Return the list of leaves
     */
    std::list<std::string> getLeafList() const;

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
    bool removeBranch(const std::string& name);

    /**
     * Remove a leaf given its name
     * \param name Leaf name
     * \return Return true if the leaf has been removed
     */
    bool removeLeaf(const std::string& name);

    /**
     * Renamed a branch
     * \param name Branch name
     * \param newName New branch name
     * \return Return true if the branch has been renamed, false if a branch by this name already exists
     */
    bool renameBranch(const std::string& name, const std::string& newName);

    /**
     * Renamed a leaf
     * \param name Leaf name
     * \param newName New leaf name
     * \return Return true if the leaf has been renamed, false if a leaf by this name already exists
     */
    bool renameLeaf(const std::string& name, const std::string& newName);

    /**
     * Set the branch name
     * \param name New branch name
     */
    void setName(const std::string& name) { _name = name; }

    /**
     * Set this branch parent
     * \param parent Parent branch
     */
    void setParent(Branch* parent) { _parentBranch = parent; }

  private:
    int _currentCallbackID{0};
    std::mutex _callbackMutex;
    std::map<Task, std::set<int>> _callbackTargetIds{};
    std::unordered_map<int, UpdateCallback> _callbacks{};
    std::string _name{"branch"};
    std::unordered_map<std::string, std::unique_ptr<Branch>> _branches{};
    std::unordered_map<std::string, std::unique_ptr<Leaf>> _leaves{};
    Branch* _parentBranch{nullptr};
};

} // namespace Tree
} // namespace Splash

#endif // SPLASH_TREE_BRANCH_H
