#if !defined(__JOB_QUEUE_H__)
#define __JOB_QUEUE_H__

#include <stdint.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>

class Event {
public:
    Event() {}
    ~Event() {}

public:
    /** シグナル状態にする.
     */
    void set();

    /** シグナル状態になるのを待つ.
     *
     * スレッドの動作が開始されるのを待つ。
     */
    void wait();

    /** 非シグナル状態にする.
     */
    void reset();

private:
    std::mutex m_mutex;
    std::condition_variable m_condVar;

    bool m_isSignal{ false };
};

class JobQueue {
private:
    static JobQueue s_instance;

    JobQueue() {}
    ~JobQueue() {}

public:
    static JobQueue& instance()
    {
        return s_instance;
    }

    using Job = std::function<void()>;

public:
    void init();
    void terminate();

    void enqueue(Job job);

    void waitAllJobs();

private:
    Job dequeue();

    enum State {
        None,
        Run,
        WillTerminate,
        Terminated,
    };
    
private:
    std::mutex m_lockJobs;
    std::queue<Job> m_jobs;

    Event m_evJobs;

    static const int THREAD_NUM = 4;

    State m_state{ State::None };
    std::thread m_threads[THREAD_NUM];
};

#endif
