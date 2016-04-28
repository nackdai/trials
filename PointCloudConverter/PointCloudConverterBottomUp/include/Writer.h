#if !defined(__WRITER_H__)
#define __WRITER_H__

#include <thread>
#include "izSystem.h"
#include "izThreadModel.h"
#include "dynamicoctree/DynamicOctree.h"
#include "Node.h"

class Writer {
public:
    Writer();
    ~Writer();

public:
    uint32_t add(const Point& obj);

    void store();
    void waitForStoring();

    void terminate();

    void flush(izanagi::threadmodel::CThreadPool& theadPool);
    void waitForFlushing();

    void close(izanagi::threadmodel::CThreadPool& theadPool);

private:
    void procStore();

private:
    DynamicOctree<Node> m_octree;

    std::vector<Point> m_objects;
    std::vector<Point> m_temporary;

    izanagi::sys::CEvent m_waitMain;

    uint64_t m_acceptedNum{ 0 };

    std::thread m_thStore;
    izanagi::sys::CEvent m_waitWorker;
    std::atomic<bool> m_runThread{ false };
};

#endif    // #if !defined(__WRITER_H__)
