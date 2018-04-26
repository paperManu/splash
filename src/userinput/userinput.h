/*
 * Copyright (C) 2016 Emmanuel Durand
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
 * @userinput.h
 * The UserInput base class
 */

#ifndef SPLASH_USERINPUT_H
#define SPLASH_USERINPUT_H

#include <memory>
#include <thread>

#include "./config.h"

#include "./core/attribute.h"
#include "./core/coretypes.h"
#include "./core/graph_object.h"
#include "./graphics/window.h"
#include "./utils/timer.h"

using namespace std;

namespace Splash
{

class UserInput : public GraphObject
{
  public:
    struct State
    {
        State() = default;
        State(const std::string& a, const Values& v = {}, int m = 0, const std::string& w = "");
        bool operator==(const State& s) const;

        std::string action{""};
        Values value{};
        int modifiers{0};
        std::string window{""};
    };

    /**
     * \brief Compares two states, only considering the action and the modifiers
     * \param lhs First state
     * \param rhs Second state
     * \return Return true if the states are considered identical
     */
    struct StateCompare
    {
        bool operator()(const State& lhs, const State& rhs) const;
    };

  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    explicit UserInput(RootObject* root);

    /**
     * \brief Destructor
     */
    virtual ~UserInput() override;

    /**
     * \brief Lock the input to the given id
     * \param id User ID to lock this input to
     */
    bool capture(const std::string& id);

    /**
     * \brief Clear the input state
     */
    void clearState() { _state.clear(); }

    /**
     * \brief Get the input state
     * \return Return the input state as vector of States
     */
    std::vector<State> getState(const std::string& id);

    /**
     * \brief Release the input
     * \param id User ID, used to check that the right user asks for release
     */
    void release(const std::string& id);

    /**
     * \brief Remove a callback
     * \param state State for which to remove a callback
     */
    static void resetCallback(const State& state);

    /**
     * \brief Set a new callback for the given state
     * This captures any state matching the given one. Only action and modifiers attributes are used for registering callbacks
     * \param state State which should trigger the callback
     * \param cb Callback
     */
    static void setCallback(const State& state, const std::function<void(const State&)>& cb);

  protected:
    std::mutex _captureMutex{}; //!< Capture mutex;
    bool _captured{false};      //!< True if the input has been captured by
    std::string _capturer{""};  //!< Id of the user locking the input

    std::mutex _stateMutex{};    //!< Mutex protecting state change
    std::vector<State> _state{}; //!< Holds the last input state

    bool _running{false};        //!< True if the update loop should be running
    int _updateRate{100};        //!< Updates per second
    std::thread _updateThread{}; //!< Thread running the update loop

    static std::mutex _callbackMutex;                                                   //!< Mutex to protect callbacks
    static std::map<State, std::function<void(const State&)>, StateCompare> _callbacks; //!< Callbacks for specific events

    /**
     * \brief Get a window name from its GLFW handler
     * \param window GLFW window handler
     * \return Return window name
     */
    std::string getWindowName(const GLFWwindow* glfwWindow) const;

    /**
     * \brief Update loop
     */
    void updateLoop();

    /**
     * \brief Callbacks update method
     */
    virtual void updateCallbacks();

    /**
     * \brief Input update method
     */
    virtual void updateMethod(){};

    /**
     * \brief State update method
     * \param state State container to update
     */
    virtual void readState(){};

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes();
};

} // end of namespace

#endif // SPLASH_USERINPUT_H
