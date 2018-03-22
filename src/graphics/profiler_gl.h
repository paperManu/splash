/*
 * Copyright (C) 2017 Jérémie Soria
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
 * @profiler_gl.h
 * Performance measurement tool
 */

#ifndef SPLASH_PROFILER_GL_H
#define SPLASH_PROFILER_GL_H

#include <algorithm>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>

namespace Splash
{

#define PROFILEGL(scope) auto _profile = std::make_unique<ProfilerGL::Section>(scope);

class ProfilerGL
{
  public:
    class Section
    {
      public:
        class Content
        {
          public:
            /**
             * \brief No default constructor
             */
            Content() = delete;
            /**
             * \brief Constructor
             * \param scope Name given to the currently profiled scope
             */
            explicit Content(const std::string& scope)
                : _scope(scope)
            {
                // We format the scope name for later use in post processing.
                std::replace(_scope.begin(), _scope.end(), ' ', '_');
            }

            // Getters/setters
            /**
             * \brief Get the name of the scope of the current section
             * \return Return the scope name as a string.
             */
            std::string getScope() const { return _scope; }
            /**
             * \brief Get the duration of OpenGL execution of the current section in nanoseconds
             * \return Return the total duration including the sum of all the profiled children sections
             */
            uint64_t getDuration() const { return _duration; }
            void setDuration(const uint64_t& duration) { _duration = duration; }
            /**
             * \brief Get the duration of OpenGL execution of the direct profiled children sections in nanoseconds
             * \return Return the cumulated duration of the execution of all the profiled children sections.
             */
            uint64_t getChildrenDuration() const { return _children_duration; }
            void setChildrenDuration(const uint64_t& children_duration) { _children_duration = children_duration; }
            /**
             * \brief Get the code depth from the highest scope profiled section
             * \return Return relative depth of the section
             */
            int getDepth() const { return _depth; }
            void setDepth(const int& depth) { _depth = depth; }

          private:
            std::string _scope{};
            uint64_t _duration{0};
            uint64_t _children_duration{0};
            int _depth{0};
        };

        struct SectionData
        {
            explicit SectionData(const std::string& scope)
                : _content(scope)
            {
            }
            Content _content;
            unsigned int _queries[2]{};
        };

        explicit Section(const std::string& scope)
            : _data(scope)
        {
            // We generate the two timers for start and end of section
            glGenQueries(2, _data._queries);

            // We keep the current code scope depth updated.
            ProfilerGL::get().increaseDepth(_data._content);

            // Get the timer at the start of the section
            glQueryCounter(_data._queries[0], GL_TIMESTAMP);
        }

        ~Section()
        {
            // Get the timer at the end of the section
            glQueryCounter(_data._queries[1], GL_TIMESTAMP);

            // We keep the current code scope depth updated.
            ProfilerGL::get().decreaseDepth(_data._content);

            // Record the closing scope for later processing to avoid CPU/GPU syncing
            ProfilerGL::get().saveSectionData(_data);
        }

      private:
        SectionData _data;
    };

    using GLTimings = std::unordered_map<std::thread::id, std::vector<Section::Content>>;
    using UnprocessedGLTimings = std::unordered_map<std::thread::id, std::vector<Section::SectionData>>;

    /**
     * \brief Default destructor
     */
    ~ProfilerGL() = default;
    /**
     * \brief No copy constructor
     */
    ProfilerGL(const ProfilerGL&) = delete;
    /**
     * \brief No move constructor
     */
    ProfilerGL(ProfilerGL&&) = delete;
    /**
     * \brief No copy assignment constructor
     */
    ProfilerGL& operator=(const ProfilerGL&) = delete;

    /**
    * \brief Get the singleton
    * \return Return the ProfilerGL singleton
    */
    static ProfilerGL& get()
    {
        static auto instance = new ProfilerGL();
        // While not technically fully thread-safe, the only way to risk a race condition is doing:
        // delete &ProfilerGL::get();
        // If you do this, please consider switching careers.
        return *instance;
    }

    /**
     * \brief Get the preprocessed timings of the currently profiled code sections
     * \return A copy of the map of profiling content for each profiled thread. We do a copy to keep the original
     * untouched in case we want to use it for multiple display methods.
     */
    GLTimings getTimings()
    {
        std::lock_guard<std::mutex> lock(_profiling_m);
        return _timings;
    }

    /**
     * \brief Record the depth of the opening profiled section relatively to the highest scope one
     * \param content Reference to the section content that will be updated
     * \param thread_id Id of the running thread
     */
    void increaseDepth(Section::Content& /*content*/)
    {
        auto thread_id = std::this_thread::get_id();
        std::lock_guard<std::mutex> lock(_processing_m);

        auto depth = _current_depth.find(thread_id);
        // Register/increment the depth of the profiled scope for the current thread
        if (depth == _current_depth.end())
            _current_depth.emplace(thread_id, 0);
        else
            ++depth->second;
    }

    /**
     * \brief Record the depth of the closing profiled section relatively to the highest scope one
     * \param content Reference to the section content that will be updated
     * \param thread_id Id of the running thread
     */
    void decreaseDepth(Section::Content& content)
    {
        auto thread_id = std::this_thread::get_id();
        std::lock_guard<std::mutex> lock(_processing_m);

        auto depth = _current_depth.find(thread_id);

        // Updates the current code depth of the current thread after the closing of a profiled section
        if (depth == _current_depth.end())
            return;
        else
        {
            content.setDepth(depth->second);
            if (depth->second > 0)
                --depth->second;
            else
                _current_depth.erase(thread_id);
        }
    }

    /**
     * \brief Copies a section data for later preprocessing through gatherTimings() call.
     * \param data Section data to be saved for later use.
     */
    void saveSectionData(Section::SectionData& data)
    {
        auto thread_id = std::this_thread::get_id();

        std::lock_guard<std::mutex> lock(_processing_m);

        auto timings = _unprocessed_timings.find(thread_id);
        if (timings == _unprocessed_timings.end())
        {
            std::vector<Section::SectionData> new_timings;
            new_timings.push_back(data);
            _unprocessed_timings.emplace(thread_id, new_timings);
        }
        else
        {
            timings->second.push_back(data);
        }
    }

    /**
     * \brief Fetches the timings of all the profiled sections data saved with saveSectionData() calls.
     */
    void gatherTimings()
    {
        auto thread_id = std::this_thread::get_id();

        std::lock_guard<std::mutex> lock(_processing_m);
        auto timings = _unprocessed_timings.find(thread_id);
        if (timings == _unprocessed_timings.end())
        {
            // Nothing to gather for this thread
            return;
        }

        for (auto& timing : timings->second)
        {
            // Wait until the query counters are available
            unsigned int timerAvailable = 0;
            while (!timerAvailable)
                glGetQueryObjectuiv(timing._queries[1], GL_QUERY_RESULT_AVAILABLE, &timerAvailable);

            // Compute the elapsed time and store it in the section content
            GLuint64 startTime, endTime;
            glGetQueryObjectui64v(timing._queries[0], GL_QUERY_RESULT, &startTime);
            glGetQueryObjectui64v(timing._queries[1], GL_QUERY_RESULT, &endTime);
            timing._content.setDuration(endTime - startTime);

            // Cleanup
            glDeleteQueries(2, timing._queries);

            // Record the section data  for postprocessing
            ProfilerGL::get().commitSection(timing._content);
        }

        timings->second.clear();
    }

    /**
     * \brief Record the closing scope for later postprocessing
     * \param content Data regarding the closing profiled code section
     */
    void commitSection(Section::Content& content)
    {
        auto thread_id = std::this_thread::get_id();

        // Stores the content of the closing profiled section for later postprocessing
        auto timings = _timings.find(thread_id);
        if (timings == _timings.end())
        {
            std::vector<Section::Content> v_content;
            v_content.push_back(content);
            _timings.emplace(thread_id, v_content);
        }
        else
        {
            timings->second.push_back(content);
        }
    }

    /**
     * \brief Process the recorded profiled sections to output in multiple formats (flamegraph, splash built-in UI)
     */
    void processTimings()
    {
        std::lock_guard<std::mutex> lock(_profiling_m);

        for (auto& timing : _timings)
        {
            // We first preprocess the cumulated time of the direct "children of each scope".
            // We need this value to compute the duration properly to format it for flamegraph.
            int depth = -1;
            std::unordered_map<int, uint64_t> children_durations;

            for (auto& content : timing.second)
            {
                auto current_depth = content.getDepth();
                if (depth > 0 && current_depth < depth)
                {
                    content.setChildrenDuration(children_durations[current_depth]);
                    children_durations[current_depth] = 0;
                }

                if (current_depth > 0)
                {
                    auto duration = children_durations.find(current_depth - 1);
                    if (duration == children_durations.end())
                        children_durations.emplace(current_depth - 1, content.getDuration());
                    else
                        duration->second += content.getDuration();
                }

                depth = current_depth;
            }
        }
    }

    /**
     * Clear the recorded timings
     */
    void clearTimings()
    {
        std::lock_guard<std::mutex> lock(_profiling_m);
        _timings.clear();
    }

    /**
     * \brief Use the timings information from the profiler and process them to print in a flamegraph (https://github.com/brendangregg/FlameGraph)
     * \param path Output file path
     */
    void processFlamegraph(const std::string& path)
    {
        // We want to prevent multiple writings to the same file
        std::lock_guard<std::mutex> lock(_processing_m);

        std::ofstream output_file;
        output_file.open(path);

        for (auto& timing : ProfilerGL::get().getTimings())
        {
            std::vector<std::string> stages;

            // Now we parse the profiling data in reverse to start from the higher level scope.
            std::reverse(timing.second.begin(), timing.second.end());
            int depth = -1;

            for (auto& content : timing.second)
            {
                auto current_depth = content.getDepth();

                // We go back one scope level plus one for each scope level between this one and the previous one.
                for (int i = 0; i <= depth - current_depth; ++i)
                    stages.pop_back();

                depth = current_depth;

                stages.push_back(content.getScope());

                for (auto& stage : stages)
                    output_file << stage << ";";

                output_file << " " << content.getDuration() - content.getChildrenDuration() << std::endl;
            }
        }

        output_file.close();
    }

  private:
    // Private constructor to prevent explicit construction
    ProfilerGL() = default;

    std::mutex _profiling_m{};
    std::mutex _processing_m{};
    std::unordered_map<std::thread::id, int> _current_depth{};
    // The timings processed by processTimings() will be stored here.
    GLTimings _timings{};
    // Query counters and profiling data will be stored here until a call to gatherTimings() is made.
    UnprocessedGLTimings _unprocessed_timings{};
};

} // namespace Splash

#endif // SPLASH_PROFILER_GL_H
