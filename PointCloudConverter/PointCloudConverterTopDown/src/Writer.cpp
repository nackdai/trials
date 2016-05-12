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

    m_objects.reserve(10000);

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
    m_objects.push_back(obj);
    return m_objects.size();
}

void Writer::store()
{
    auto num = m_objects.size();

    if (num == 0) {
        return;
    }

    //m_flush.wait();
    
    m_store.wait(true);

    m_temporary.resize(m_objects.size());

    memcpy(
        &m_temporary[0],
        &m_objects[0],
        sizeof(Point) * num);

    m_objects.clear();

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

    for (auto obj : m_temporary) {
        // TODO
        auto n = izanagi::math::CMathRand::GetRandBetween(0, 100);
#if 0
        IZ_UINT targetLevel = n / step;

        auto node = m_octree.getNode(0);

        if (targetLevel == 0 && !node->canRegister(level - 1)) {
            targetLevel++;
        }

        for (IZ_UINT i = 0; i < targetLevel; i++) {
            Node** nodes = m_octree.getChildren(node);

            if (nodes) {
                for (IZ_UINT n = 0; n < 8; n++) {
                    node = nodes[n];

                    bool canRegister = node->canRegister(level - 1);

                    if (canRegister) {
                        if (node->isContain(obj)) {
                            break;
                        }
                    }
                    else {
                        targetLevel = (targetLevel < level ? targetLevel + 1 : level);
                    }
                }
            }
            else {
                break;
            }
        }
#else
        n = table[n];

        IZ_UINT targetLevel = n / step;

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
#endif

        node->add(obj);

        m_acceptedNum++;
    }

    m_temporary.clear();
}

void Writer::flush(izanagi::threadmodel::CThreadPool& theadPool)
{
    m_flush.wait(true);

    //m_store.wait();

    m_flush.set();
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
    auto num = m_objects.size();

    if (num == 0) {
        return;
    }

    m_store.wait();

    m_temporary.resize(m_objects.size());

    memcpy(
        &m_temporary[0],
        &m_objects[0],
        sizeof(Point) * num);

    m_objects.clear();

    procStore();
}

void Writer::flushDirectly()
{
    m_flush.wait();

    procFlush();
}

void Writer::close(izanagi::threadmodel::CThreadPool& theadPool)
{
    m_store.wait();
    m_flush.wait();

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
