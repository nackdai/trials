#include "JobQueue.h"

// シグナル状態にする.
void Event::set()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_isSignal = true;

    m_condVar.notify_all();
}

// シグナル状態になるのを待つ.
void Event::wait()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condVar.wait(
        lock,
        [this] {
        return m_isSignal;
    });
}

// 非シグナル状態にする.
void Event::reset()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_isSignal = false;
}

//////////////////////////////////////////////////

JobQueue JobQueue::s_instance;

void JobQueue::init()
{
    m_state = State::Run;

    for (int i = 0; i < _countof(m_threads); i++) {
        m_threads[i] = std::thread([&] {
            while (m_state == State::Run) {
                m_evJobs.wait();
                auto job = dequeue();
                job();
            }
        });
    }
}

void JobQueue::terminate()
{
    m_state = State::WillTerminate;

    for (int i = 0; i < _countof(m_threads); i++) {
        if (m_threads[i].joinable()) {
            m_threads[i].join();
        }
    }

    m_state = State::Terminated;
}

void JobQueue::enqueue(Job job)
{
    std::unique_lock<std::mutex> lock(m_lockJobs);
    m_jobs.push(job);

    m_evJobs.set();
}

void JobQueue::waitAllJobs()
{
    std::unique_lock<std::mutex> lock(m_lockJobs);

    // TODO
    for (;;) {
        if (m_jobs.size() == 0) {
            break;
        }
    }
}

JobQueue::Job JobQueue::dequeue()
{
    std::unique_lock<std::mutex> lock(m_lockJobs);
    auto job = m_jobs.front();
    m_jobs.pop();

    if (m_jobs.size() == 0) {
        m_evJobs.reset();
    }

    return job;
}
