#include "./threadpool.h"

#include <algorithm>
#include <unistd.h>

#include "./log.h"
#include "./osUtils.h"

using namespace std;
using namespace Splash;

/*************/
void Worker::operator()()
{
    shared_ptr<function<void()>> task;
    Utils::setRealTime();

    while (true)
    {
        if (!affinitySet && pool.cores.size() > 0)
        {
            Utils::setAffinity(pool.cores);
            affinitySet = true;
        }

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
            lock_guard<mutex> lock(pool.queue_mutex);
            if (id != 0)
                pool.tasksFinished.push_back(id);
        }
    }
}

/*************/
ThreadPool::ThreadPool(int threads)
{
    int nprocessors = threads;
    if (threads == -1)
        nprocessors = std::min(std::max(thread::hardware_concurrency(), 2u), 32u);
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
void ThreadPool::setAffinity(const vector<int>& cores)
{
    this->cores = cores;
}

/*************/
void ThreadPool::waitAllThreads()
{
    while (true)
    {
        this_thread::sleep_for(chrono::nanoseconds((unsigned long)1e5));
        if (workingThreads == 0 && tasks.size() == 0)
        {
            lock_guard<mutex> lock(queue_mutex);
            tasksFinished.clear();
            break;
        }
    }
}

/*************/
void ThreadPool::waitThreads(vector<unsigned int> list)
{
    while (true)
    {
        this_thread::sleep_for(chrono::microseconds(100));

        lock_guard<mutex> lock(queue_mutex);
        if (list.size() == 0 || stop == true)
        {
            break;
        }

        while (true)
        {
            auto task = find(tasksFinished.begin(), tasksFinished.end(), list[0]);
            if (task != tasksFinished.end())
            {
                list.erase(list.begin());
                tasksFinished.erase(task);
            }
            else
            {
                break;
            }
        }
    }
}

/*************/
unsigned int ThreadPool::getTasksNumber()
{
    int size;
    {
        lock_guard<mutex> lock(queue_mutex);
        size = tasks.size();
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
