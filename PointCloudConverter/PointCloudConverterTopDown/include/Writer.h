#if !defined(__WRITER_H__)
#define __WRITER_H__

#include "proxy.h"

#include <thread>
#include "izSystem.h"
#include "izThreadModel.h"
#include "Node.h"

class Writer {
public:
    Writer(
        izanagi::IMemoryAllocator* allocator,
        const Potree::AABB& aabb);
    ~Writer();

public:
    uint32_t add(const Point& obj);

    void store();

    void terminate();

    void flush(izanagi::threadmodel::CThreadPool& theadPool);

    void close(izanagi::threadmodel::CThreadPool& theadPool);

private:
    void procStore();
    void procFlush();

private:
    izanagi::col::Octree<Node> m_octree;

    std::vector<Point> m_objects;
    std::vector<Point> m_temporary;

    class Worker {
    public:
        Worker() {}
        ~Worker() {}

    public:
        void start(std::function<void(void)> func);

        void set();
        void wait(bool reset = false);

        void join();

    private:
        std::function<void(void)> m_func;

        izanagi::sys::CEvent m_waitMain;

        std::thread m_thread;
        izanagi::sys::CEvent m_waitWorker;
        std::atomic<bool> m_runThread{ false };
    };

    uint64_t m_acceptedNum{ 0 };

    Worker m_store;
    Worker m_flush;
};

#endif    // #if !defined(__WRITER_H__)
