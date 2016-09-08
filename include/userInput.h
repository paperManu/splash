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
 * @userInput.h
 * The UserInput base class
 */

#ifndef SPLASH_USERINPUT_H
#define SPLASH_USERINPUT_H

#include <memory>
#include <thread>

#include "./config.h"

#include "./basetypes.h"
#include "./controller.h"
#include "./coretypes.h"
#include "./timer.h"
#include "./window.h"

using namespace std;

namespace Splash
{

class UserInput : public ControllerObject
{
  public:
    struct State
    {
        std::string action{""};
        Values value{};
        int modifiers{0};
        std::string window{""};
    };

  public:
    /**
     * \brief Constructor
     * \param root Root object
     */
    UserInput(std::weak_ptr<RootObject> root) : ControllerObject(root)
    {
        _type = "userInput";
        registerAttributes();
        _updateThread = std::thread([&]() {
            _running = true;
            updateLoop();
        });
    }

    /**
     * \brief Destructor
     */
    virtual ~UserInput()
    {
        _running = false;
        if (_updateThread.joinable())
            _updateThread.join();
    }

    /**
     * \brief Lock the input to the given id
     * \param id User ID to lock this input to
     */
    bool capture(std::string id)
    {
        std::lock_guard<std::mutex> lock(_captureMutex);
        if (_captured)
            return false;

        _captured = true;
        _capturer = id;
        return true;
    }

    /**
     * \brief Clear the input state
     */
    void clearState() { _state.clear(); }

    /**
     * \brief Get the input state
     * \return Return the input state as vector of States
     */
    std::vector<State> getState(std::string id)
    {
        std::lock_guard<std::mutex> lockState(_stateMutex);
        std::lock_guard<std::mutex> lockCapture(_captureMutex);

        if (_captured && _capturer != id)
            return {};

        readState();

        auto state(_state);
        _state.clear();
        return state;
    }

    /**
     * \brief Release the input
     * \param id User ID, used to check that the right user asks for release
     */
    void release(std::string id)
    {
        std::lock_guard<std::mutex> lock(_captureMutex);
        if (id == _capturer)
        {
            _captured = false;
            _capturer = "";
        }
    }

  protected:
    std::mutex _captureMutex{}; //!< Capture mutex;
    bool _captured{false};      //!< True if the input has been captured by
    std::string _capturer{""};  //!< Id of the user locking the input

    std::mutex _stateMutex{};    //!< Mutex protecting state change
    std::vector<State> _state{}; //!< Holds the last input state

    bool _running{false};        //!< True if the update loop should be running
    int _updateRate{100};        //!< Updates per second
    std::thread _updateThread{}; //!< Thread running the update loop

    /**
     * \brief Get a window name from its GLFW handler
     * \param window GLFW window handler
     * \return Return window name
     */
    std::string getWindowName(GLFWwindow* glfwWindow)
    {
        auto windows = getObjectsOfType("window");
        for (auto& w : windows)
        {
            shared_ptr<Window> window = dynamic_pointer_cast<Window>(w);
            if (window->isWindow(glfwWindow))
                return window->getName();
        }

        return "";
    }

    /**
     * \brief Update loop
     */
    void updateLoop()
    {
        while (_running)
        {
            auto start = Timer::getTime();
            {
                std::lock_guard<std::mutex> lock(_stateMutex);
                updateMethod();
            }
            auto end = Timer::getTime();
            auto delta = end - start;
            int64_t loopDuration = 1e6 / _updateRate;
            std::this_thread::sleep_for(std::chrono::microseconds(loopDuration - delta));
        }
    }

    /**
     * \brief Input update method
     */
    virtual void updateMethod() = 0;

    /**
     * \brief State update method
     */
    virtual void readState(){};

    /**
     * Register new functors to modify attributes
     */
    void registerAttributes()
    {
        addAttribute("updateRate",
            [&](const Values& args) {
                _updateRate = std::max(10, args[0].asInt());
                return true;
            },
            [&]() -> Values { return {_updateRate}; },
            {'n'});
        setAttributeDescription("updateRate", "Set the rate at which the inputs are updated");
    }
};

} // end of namespace

#endif // SPLASH_USERINPUT_H
