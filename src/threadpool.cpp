#include "threadpool.h"

#include <algorithm>
#include <unistd.h>

using namespace std;

/*************/
void Worker::operator() ()
{
    shared_ptr<function<void()>> task;

    while (true)
    {
        unsigned int id;
        {
            // Acquire locklock
            unique_lock<mutex> lock(pool.queue_mutex);

            // look for a work item
            while (!pool.stop && pool.tasks.empty())
                pool.condition.wait(lock);

            // exit the pool if stopped
            if (pool.stop)
                return;

            task.swap(pool.tasks.front());
            pool.tasks.pop_front();

            id = pool.tasksId.front();
            pool.tasksId.pop_front();
        }   

        // Execute the task
        pool.workingThreads++;
        (*task)();
        pool.workingThreads--;

        {
            pool.queue_mutex.lock();
            if (id != 0)
                pool.tasksFinished.push_back(id);
            pool.queue_mutex.unlock();
        }
    }
}

/*************/
ThreadPool::ThreadPool(int threads)
{
    int nprocessors = threads;
    if (threads == -1)
        nprocessors = std::min(std::max(sysconf(_SC_NPROCESSORS_CONF), 2l), 16l);
    for (size_t i = 0; i < nprocessors; ++i)
        workers.emplace_back(thread(Worker(*this)));
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
    while (true)
    {
        this_thread::sleep_for(chrono::nanoseconds((unsigned long)1e5));
        if (workingThreads == 0 && tasks.size() == 0)
        {
            queue_mutex.lock();
            tasksFinished.clear();
            queue_mutex.unlock();
            break;
        }
    }
}

/*************/
void ThreadPool::waitThreads(vector<unsigned int>& list)
{
    timespec nap;
    nap.tv_sec = 0;
    nap.tv_nsec = 1e3;

    while (true)
    {
        nanosleep(&nap, NULL);

        queue_mutex.lock();
        if (list.size() == 0 || stop == true)
        {
            queue_mutex.unlock();
            break;
        }

        auto task = find(tasksFinished.begin(), tasksFinished.end(), list[0]);
        if (task != tasksFinished.end())
        {
            list.erase(list.begin());
            tasksFinished.erase(task);
        }
        queue_mutex.unlock();
    }
}

/*************/
unsigned int ThreadPool::getTasksNumber()
{
    int size;
    {
        queue_mutex.lock();
        size = tasks.size();
        queue_mutex.unlock();
    }
    return size;
}

/*************/
void ThreadPool::addWorkers(unsigned int nbr)
{
    for (unsigned int i = 0; i < nbr; ++i)
        workers.emplace_back(thread(Worker(*this)));
}

/*************/
ThreadPool SThread::pool;
