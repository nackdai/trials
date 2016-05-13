#if !defined(__WRITER_H__)
#define __WRITER_H__

#include "proxy.h"

#include <thread>
#include "izSystem.h"
#include "izThreadModel.h"
#include "Node.h"

//#define USE_THREAD_FLUSH
#define USE_THREAD_STORE

class Writer {
public:
    Writer(
        izanagi::IMemoryAllocator* allocator,
        const Potree::AABB& aabb,
        uint32_t depth);
    ~Writer();

public:
    uint32_t add(const Point& obj);

    void store(izanagi::threadmodel::CThreadPool& threadPool);

    void terminate();

    void flush(izanagi::threadmodel::CThreadPool& threadPool);

    void close(izanagi::threadmodel::CThreadPool& threadPool);

    void storeDirectly(izanagi::threadmodel::CThreadPool& threadPool);
    void flushDirectly(izanagi::threadmodel::CThreadPool& threadPool);

private:
    void procStore();
    void procFlush();

private:
    izanagi::col::Octree<Node> m_octree;

    std::vector<Point> m_objects[2];
    uint32_t m_curIdx{ 0 };

    std::vector<Point>* m_temporary{ nullptr };

    class Worker {
    public:
        Worker(const std::string& tag) : m_tag(tag) {}
        ~Worker() {}

    public:
        void start(std::function<void(void)> func);

        void set();
        void wait(bool reset = false);

        void join();

    private:
        std::string m_tag;

        std::function<void(void)> m_func;

        izanagi::sys::CEvent m_waitMain;

        std::thread m_thread;
        izanagi::sys::CEvent m_waitWorker;
        std::atomic<bool> m_runThread{ false };
    };

    std::atomic<uint64_t> m_acceptedNum{ 0 };

    Worker m_store;
    Worker m_flush;

    izanagi::threadmodel::CParallelFor m_storeTasks[10];
    izanagi::threadmodel::CParallelFor m_flushTasks[10];

    izanagi::sys::CSpinLock m_locker;
};

#endif    // #if !defined(__WRITER_H__)
