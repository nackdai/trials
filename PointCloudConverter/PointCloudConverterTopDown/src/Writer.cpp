#include "Writer.h"

///////////////////////////////////////////////////////

void Writer::Worker::start(std::function<void(void)> func)
{
    m_runThread = true;

    m_waitMain.Set();

    m_func = func;

    m_thread = std::thread([&] {
        while (m_runThread) {
            m_waitWorker.Wait();
            m_waitWorker.Reset();

            if (m_runThread) {
                //izanagi::_OutputDebugString("Running(%s)============\n", m_tag.c_str());

                m_func();

                //izanagi::_OutputDebugString("Done(%s)===============\n", m_tag.c_str());
            }

            m_waitMain.Set();
        }
    });
}

void Writer::Worker::set()
{
    m_waitWorker.Set();
}

void Writer::Worker::wait(bool reset/*= false*/)
{
    m_waitMain.Wait();
    if (reset) {
        m_waitMain.Reset();
    }
}

void Writer::Worker::join()
{
    m_runThread = false;

    m_waitWorker.Set();

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

///////////////////////////////////////////////////////

Writer::Writer(
    izanagi::IMemoryAllocator* allocator,
    const Potree::AABB& aabb)
    : m_store("store"), m_flush("flush")
{
    const auto& min = aabb.min;
    const auto& max = aabb.max;
    
    m_octree.initialize(
        allocator,
        izanagi::math::CVector4(min.x, min.y, min.z),
        izanagi::math::CVector4(max.x, max.y, max.z),
        3);

    m_objects[0].reserve(10000);
    m_objects[1].reserve(10000);

    m_flush.start([&] {
        procFlush();
    });

    m_store.start([&] {
        procStore();
    });
}

Writer::~Writer()
{
    terminate();
}

uint32_t Writer::add(const Point& obj)
{
    m_objects[m_curIdx].push_back(obj);
    return m_objects[m_curIdx].size();
}

void Writer::store()
{
    auto num = m_objects[m_curIdx].size();

    if (num == 0) {
        return;
    }

    //m_flush.wait();
    
    m_store.wait(true);

    m_temporary = &m_objects[m_curIdx];

    m_curIdx = 1 - m_curIdx;

    m_store.set();
}

void Writer::terminate()
{   
    m_store.join();
    m_flush.join();
}

static int table[] = {
#if 0
#include "sintbl.dat"
#elif 0
#include "quadtbl.dat"
#else
#include "easetbl.dat"
#endif
};

void Writer::procStore()
{
    auto level = m_octree.getMaxLevel();

    auto step = 100 / level;

    auto& points = *m_temporary;

    for (auto obj : points) {
        // TODO
        auto n = izanagi::math::CMathRand::GetRandBetween(0, 100);
        n = table[n];

        IZ_UINT targetLevel = n / step;

#if 0
        auto node = m_octree.getNode(0);

        for (IZ_UINT i = 0; i < targetLevel; i++) {
            Node** nodes = m_octree.getChildren(node);

            if (nodes) {
                for (IZ_UINT n = 0; n < 8; n++) {
                    node = nodes[n];

                    if (node->isContain(obj)) {
                        break;
                    }
                }
            }
            else {
                break;
            }
        }
#else
        targetLevel = IZ_MIN(targetLevel, level - 1);

        auto mortonNumber = m_octree.getMortonNumberByLevel(
            izanagi::math::CVector4(obj.pos[0], obj.pos[1], obj.pos[2]),
            targetLevel);

        auto idx = m_octree.getIndex(mortonNumber);

        auto node = m_octree.getNode(idx, IZ_TRUE);
        IZ_ASSERT(node->isContain(obj));
#endif

        node->add(obj);

        m_acceptedNum++;
    }

    points.clear();
}

void Writer::flush(izanagi::threadmodel::CThreadPool& theadPool)
{
#ifdef USE_THREAD_FLUSH
    m_flush.wait(true);

    //m_store.wait();

    m_flush.set();
#else
    izanagi::threadmodel::CParallel::waitFor();

    auto num = m_octree.getNodeCount();
    
    izanagi::threadmodel::CParallel::For(
        theadPool,
        0, num,
        [&](IZ_UINT idx) {
        auto list = m_octree.getNodes();
        Node* node = list[idx];
        if (node) {
            node->flush();
        }
    });
#endif
}

void Writer::procFlush()
{
    auto list = m_octree.getNodes();
    auto num = m_octree.getNodeCount();

    izanagi::sys::CTimer timer;

    timer.Begin();
    for (IZ_UINT i = 0; i < num; i++) {
        Node* node = list[i];
        if (node) {
            node->flush();
        }
    }
    auto time = timer.End();
    izanagi::_OutputDebugString("****** Flush - %f(ms)\n", time);
}

void Writer::storeDirectly()
{
    auto num = m_objects[m_curIdx].size();

    if (num == 0) {
        return;
    }

    m_store.wait();

    m_temporary = &m_objects[m_curIdx];

    m_curIdx = 1 - m_curIdx;

    procStore();
}

void Writer::flushDirectly(izanagi::threadmodel::CThreadPool& theadPool)
{
#ifdef USE_THREAD_FLUSH
    m_flush.wait();

    procFlush();
#else
    flush(theadPool);
    izanagi::threadmodel::CParallel::waitFor();
#endif
}

void Writer::close(izanagi::threadmodel::CThreadPool& theadPool)
{
    m_store.wait();

#ifdef USE_THREAD_FLUSH
    m_flush.wait();
#else
    izanagi::threadmodel::CParallel::waitFor();
#endif

    auto list = m_octree.getNodes();
    auto num = m_octree.getNodeCount();

#if 0
    for (IZ_UINT i = 0; i < num; i++) {
        Node* node = list[i];
        if (node) {
            node->close();
        }
    }
#else
    izanagi::threadmodel::CParallel::For(
        theadPool,
        0, num,
        [&](IZ_INT idx) {
        Node* node = list[idx];
        if (node) {
            node->close();
        }
    });
    izanagi::threadmodel::CParallel::waitFor();
#endif
}
