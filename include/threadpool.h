// Based (massively) on this implementation :
// http://progsch.net/wordpress/?p=81

// Zlib license:
// Copyright (c) <2012> <Jakob Progsch>
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
//    2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 
//    3. This notice may not be removed or altered from any source
//    distribution.

#ifndef SPLASH_THREADPOOL_H
#define SPLASH_THREADPOOL_H

#include <atomic>
#include <iostream>
#include <vector>
#include <deque>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "config.h"

#define SPLASH_MAX_THREAD 8

/*************/
class ThreadPool;

/*************/
class Worker
{
    public:
        Worker(ThreadPool &s) : pool(s) { }
        void operator() ();
     
    private:
        ThreadPool &pool;
};

/*************/
class ThreadPool
{
    public:
        ThreadPool(int threads = -1);
        ~ThreadPool();

        template<class F> unsigned int enqueue(F f);
        template<class F> void enqueueWithoutId(F f);
        unsigned int getTasksNumber();
        void addWorkers(unsigned int nbr);
        void waitAllThreads();
        void waitThreads(std::vector<unsigned int>&);

    private:
        friend class Worker;

        std::vector<std::thread> workers;
        std::deque<std::shared_ptr<std::function<void()>>> tasks;
        std::deque<unsigned int> tasksId;
        std::deque<unsigned int> tasksFinished;

        std::atomic_uint workingThreads {0};
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop {false};

        std::atomic_uint nextId {1};
};

typedef std::shared_ptr<ThreadPool> ThreadPoolPtr;

/*************/
template<class F>
unsigned int ThreadPool::enqueue(F f)
{
    unsigned int id;
    {
        // Acquire lock
        queue_mutex.lock();

        id = std::atomic_fetch_add(&nextId, 1u);
        if (id == std::numeric_limits<unsigned int>::max())
            nextId = 1;

        // Add the task
        tasks.push_back(std::shared_ptr<std::function<void()>>(new std::function<void()>(f)));
        tasksId.push_back(id);

        queue_mutex.unlock();
    }

    // Wake up one thread
    condition.notify_one();

    return id;
}

/*************/
template<class F>
void ThreadPool::enqueueWithoutId(F f)
{
    {
        // Acquire lock
        queue_mutex.lock();

        // Add the task
        tasks.push_back(std::shared_ptr<std::function<void()>>(new std::function<void()>(f)));
        tasksId.push_back(0);

        queue_mutex.unlock();
    }

    // Wake up one thread
    condition.notify_one();
}

/*************/
// Global thread pool
struct SThread
{
    public:
        static ThreadPool pool;
};

#endif // SPLASH_THREADPOOL_H
