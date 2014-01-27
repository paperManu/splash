#include "threadpool.h"

#include <algorithm>

using namespace std;

/*************/
void Worker::operator() ()
{
    function<void()> task;

    while (true)
    {
        unsigned int id;
        {
            // Acquire lock
            unique_lock<mutex> lock(pool.queue_mutex);

            // look for a work item
            while (!pool.stop && pool.tasks.empty())
                pool.condition.wait(lock);

            // exit the pool if stopped
            if (pool.stop)
                return;

            task = pool.tasks.front();
            pool.tasks.pop_front();

            id = pool.tasksId.front();
            pool.tasksId.pop_front();

        }   // Release lock

        // Execute the task
        pool.workingThreads++;
        task();
        pool.workingThreads--;

        {
            unique_lock<mutex> lock(pool.queue_mutex);
            pool.tasksFinished.push_back(id);
        }
    }
}

/*************/
ThreadPool::ThreadPool(size_t threads)
    : stop(false)
{
    workingThreads = 0;

    for (size_t i = 0; i < threads; ++i)
        workers.push_back(thread(Worker(*this)));
}

/*************/
ThreadPool::~ThreadPool()
{
    // Stop all threads
    stop = true;
    condition.notify_all();

    // join them
    for (size_t i = 0; i < workers.size(); ++i)
        workers[i].join();
}

/*************/
void ThreadPool::waitAllThreads()
{
    timespec nap;
    nap.tv_sec = 0;
    nap.tv_nsec = 1e5;

    while (true)
    {
        nanosleep(&nap, NULL);
        if (workingThreads == 0 && tasks.size() == 0)
        {
            unique_lock<mutex> lock(queue_mutex);
            tasksFinished.clear();
            break;
        }
    }
}

/*************/
void ThreadPool::waitThreads(vector<unsigned int> list)
{
    timespec nap;
    nap.tv_sec = 0;
    nap.tv_nsec = 1e4;

    while (true)
    {
        nanosleep(&nap, NULL);

        unique_lock<mutex> lock(queue_mutex);
        if (list.size() == 0 || workingThreads == 0 || tasksFinished.size() == 0)
        {
            tasksFinished.clear();
            break;
        }

        auto task = find(tasksFinished.begin(), tasksFinished.end(), list[0]);
        if (task != tasksFinished.end())
        {
            list.erase(list.begin());
            tasksFinished.erase(task);
        }
    }
}

/*************/
unsigned int ThreadPool::getPoolLength()
{
    int size;
    {
        unique_lock<mutex> lock(queue_mutex);
        size = tasks.size();
    }
    return size;
}

/*************/
ThreadPool SThread::pool(SPLASH_MAX_THREAD);
