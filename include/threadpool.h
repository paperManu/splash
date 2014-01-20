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

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <atomic>
#include <iostream>
#include <vector>
#include <deque>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "config.h"
#include "coretypes.h"

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
        ThreadPool(size_t);
        ~ThreadPool();

        template<class F> unsigned int enqueue(F f);
        void waitAllThreads();
        void waitThreads(std::vector<unsigned int>);
        unsigned int getPoolLength();

    private:
        friend class Worker;

        std::vector<std::thread> workers;
        std::deque<std::function<void()> > tasks;
        std::deque<unsigned int> tasksId;
        std::deque<unsigned int> tasksFinished;

        std::atomic_uint workingThreads;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;

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
        std::unique_lock<std::mutex> lock(queue_mutex);

        id = std::atomic_fetch_add(&nextId, 1u);

        // Add the task
        tasks.push_back(std::function<void()>(f));
        tasksId.push_back(id);
    }

    // Wake up one thread
    condition.notify_one();

    return id;
}

/*************/
// Global thread pool
struct SThread
{
    public:
        static ThreadPool pool;
};

#endif // THREADPOOL_H
