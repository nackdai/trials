#if !defined(__WRITER_H__)
#define __WRITER_H__

#include "proxy.h"

#include <thread>
#include "izSystem.h"
#include "izThreadModel.h"
#include "Node.h"
#include "Octree.h"
#include "Config.h"

#define USE_THREAD_FLUSH
#define USE_THREAD_STORE
#define USE_THREAD_RAND

class Writer {
public:
    Writer(
        izanagi::IMemoryAllocator* allocator,
        const Potree::AABB& aabb,
        uint32_t depth);
    ~Writer();

public:
    uint32_t add(const Point& obj);

    Point& getNextPoint()
    {
        IZ_ASSERT(m_registeredNum < STORE_LIMIT);

        auto& ret = m_objects[m_curIdx][m_registeredNum++];
        return ret;
    }

    void store(izanagi::threadmodel::CThreadPool& threadPool);
    void store();

    void terminate();

    void flush(izanagi::threadmodel::CThreadPool& threadPool);

    void close(izanagi::threadmodel::CThreadPool& threadPool);

    void storeDirectly(izanagi::threadmodel::CThreadPool& threadPool);
    void flushDirectly(izanagi::threadmodel::CThreadPool& threadPool);

    void* getBuffer()
    {
        return m_objects[m_curIdx];
    }

    void merge(
        Writer* writer[],
        uint32_t num,
        izanagi::threadmodel::CThreadPool& threadPool);

private:
    void procStore(bool runRand = true);
    void procFlush();

    void procRand();

public:
    Octree m_octree;

    Point m_objects[2][STORE_LIMIT];

    uint32_t m_registeredNum{ 0 };
    uint32_t m_willStoreNum{ 0 };

    uint32_t m_curIdx{ 0 };

    Point* m_temporary{ nullptr };

    class Worker {
    public:
        Worker(const std::string& tag) : m_tag(tag) {}
        ~Worker() {}

    public:
        void start(std::function<void(void)> func);

        void set();
        void wait(bool reset = false);

        void join();

        void setMain();

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

#ifdef USE_THREAD_FLUSH
    Worker m_flush;
#endif

#ifdef USE_THREAD_RAND
    Worker m_rand;
    uint32_t m_levels[STORE_LIMIT];
#endif

    izanagi::threadmodel::CParallelFor m_storeTasks[10];
    izanagi::threadmodel::CParallelFor m_flushTasks[10];

    izanagi::sys::CSpinLock m_locker;
};

#endif    // #if !defined(__WRITER_H__)
