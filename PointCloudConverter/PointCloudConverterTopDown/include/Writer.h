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
    void waitForFlushing();

    void close(izanagi::threadmodel::CThreadPool& theadPool);

private:
    void procStore();

private:
    izanagi::col::Octree<Node> m_octree;

    std::vector<Point> m_objects;
    std::vector<Point> m_temporary;

    izanagi::sys::CEvent m_waitMain;

    uint64_t m_acceptedNum{ 0 };

    std::thread m_thStore;
    izanagi::sys::CEvent m_waitWorker;
    std::atomic<bool> m_runThread{ false };
};

#endif    // #if !defined(__WRITER_H__)
