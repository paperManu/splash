#include "threadpool.h"

/*************/
void Worker::operator() ()
{
    std::function<void()> task;

    while (true)
    {
        {
            // Acquire lock
            std::unique_lock<std::mutex> lock(pool.queue_mutex);

            // Mark as available

            // look for a work item
            while (!pool.stop && pool.tasks.empty())
                pool.condition.wait(lock);

            // exit the pool if stopped
            if (pool.stop)
                return;

            task = pool.tasks.front();
            pool.tasks.pop_front();

            // Mark as working

        }   // Release lock

        // Execute the task
        pool.workingThreads++;
        task();
        pool.workingThreads--;
    }
}

/*************/
ThreadPool::ThreadPool(size_t threads)
    : stop(false)
{
    workingThreads = 0;

    for (size_t i = 0; i < threads; ++i)
        workers.push_back(std::thread(Worker(*this)));
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

    bool waiting = true;
    while (waiting)
    {
        nanosleep(&nap, NULL);
        {
            if (workingThreads == 0 && tasks.size() == 0)
                waiting = false;
        }
    }
}

/*************/
unsigned int ThreadPool::getPoolLength()
{
    int size;
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        size = tasks.size();
    }
    return size;
}

/*************/
// One threadpool to do it all
ThreadPoolPtr SThreadPool::pool(new ThreadPool(SPLASH_MAX_THREAD));
