#include "Writer.h"

static uint32_t table[] = {
#if 0
#include "sintbl.dat"
#elif 0
#include "quadtbl.dat"
#else
#include "easetbl.dat"
#endif
};

///////////////////////////////////////////////////////

void Writer::Worker::start(std::function<void(void)> func)
{
    m_runThread = true;

    //m_waitMain.Set();

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

void Writer::Worker::setMain()
{
    m_waitMain.Set();
}

///////////////////////////////////////////////////////

Writer::Writer(
    izanagi::IMemoryAllocator* allocator,
    const Potree::AABB& aabb,
    uint32_t depth)
    : m_store("store")
#ifdef USE_THREAD_FLUSH
    , m_flush("flush")
#endif
#ifdef USE_THREAD_RAND
    , m_rand("rand")
#endif
{
    const auto& min = aabb.min;
    const auto& max = aabb.max;
    
    m_octree.initialize(
        izanagi::math::CVector4(min.x, min.y, min.z),
        izanagi::math::CVector4(max.x, max.y, max.z),
        depth);

    auto level = m_octree.getMaxLevel();

    auto step = 100 / level;
    step++;

    for (uint32_t i = 0; i < COUNTOF(table); i++) {
        table[i] /= step;
    }

#ifdef USE_THREAD_FLUSH
    m_flush.start([&] {
        procFlush();
    });

    m_flush.setMain();
#endif

    m_store.start([&] {
        procStore();
    });

    m_store.setMain();

#ifdef USE_THREAD_RAND
    m_rand.start([&] {
        procRand();
    });

    m_rand.set();
#endif
}

Writer::~Writer()
{
    terminate();
}

uint32_t Writer::add(const Point& obj)
{
    IZ_ASSERT(m_registeredNum < STORE_LIMIT);

    m_objects[m_curIdx][m_registeredNum++] = obj;
    return m_registeredNum;
}

void Writer::store(izanagi::threadmodel::CThreadPool& threadPool)
{
    auto num = m_registeredNum;

    if (num == 0) {
        return;
    }

#ifdef USE_THREAD_STORE
    //m_flush.wait();
    
    m_store.wait(true);

#ifdef USE_THREAD_RAND
    m_rand.wait(true);
#endif

    auto lastIdx = 1 - m_curIdx;
    m_willStoreNum = m_registeredNum;
    m_registeredNum = 0;

    m_temporary = m_objects[m_curIdx];

    m_curIdx = 1 - m_curIdx;

    m_store.set();
#else
    izanagi::threadmodel::CParallel::waitFor(m_storeTasks, COUNTOF(m_storeTasks));

    auto lastIdx = 1 - m_curIdx;
    m_objects[lastIdx].clear();

    m_temporary = &m_objects[m_curIdx];

    m_curIdx = 1 - m_curIdx;

    auto& points = *m_temporary;

    izanagi::threadmodel::CParallel::For(
        threadPool,
        m_storeTasks,
        0, num,
        [&](IZ_UINT idx) {
        auto& obj = points[idx];

        auto n = izanagi::math::CMathRand::GetRandBetween(0, 100);
        n = table[n];

        auto level = m_octree.getMaxLevel();
        auto step = 100 / level;

        IZ_UINT targetLevel = n / step;

        targetLevel = IZ_MIN(targetLevel, level - 1);

        auto mortonNumber = m_octree.getMortonNumberByLevel(
            izanagi::math::CVector4(obj.pos[0], obj.pos[1], obj.pos[2]),
            targetLevel);

        auto nodeIdx = m_octree.getIndex(mortonNumber);

        m_locker.lock();

        auto node = m_octree.getNode(nodeIdx, IZ_TRUE);
        IZ_ASSERT(node->isContain(obj));

        node->add(obj);

        m_locker.unlock();

        m_acceptedNum++;
    });
#endif
}

void Writer::store()
{
    auto num = m_registeredNum;

    if (num == 0) {
        return;
    }

#ifdef USE_THREAD_RAND
    m_rand.wait();
#endif

    m_temporary = m_objects[m_curIdx];

    m_willStoreNum = m_registeredNum;
    m_registeredNum = 0;

    m_curIdx = 1 - m_curIdx;

    procStore();
}

void Writer::terminate()
{   
    m_store.join();

#ifdef USE_THREAD_FLUSH
    m_flush.join();
#endif

#ifdef USE_THREAD_RAND
    m_rand.join();
#endif
}

void Writer::procStore(bool runRand/*= true*/)
{
    auto level = m_octree.getMaxLevel();

    auto step = 100 / level;
    step++;

    auto points = m_temporary;
    auto levels = m_levels;

    int loopCnt = m_willStoreNum;

    MortonNumber mortonNumber0;
    MortonNumber mortonNumber1;
    MortonNumber mortonNumber2;
    MortonNumber mortonNumber3;

    while (loopCnt >= 4)
    {
        auto idx0 = loopCnt - 1;
        auto idx1 = loopCnt - 2;
        auto idx2 = loopCnt - 3;
        auto idx3 = loopCnt - 4;

        const auto& obj0 = points[idx0];
        const auto& obj1 = points[idx1];
        const auto& obj2 = points[idx2];
        const auto& obj3 = points[idx3];

#ifdef USE_THREAD_RAND
        IZ_UINT targetLevel0 = levels[idx0];
        IZ_UINT targetLevel1 = levels[idx1];
        IZ_UINT targetLevel2 = levels[idx2];
        IZ_UINT targetLevel3 = levels[idx3];
#else
        double f = izanagi::math::CMathRand::GetRandFloat() * 100.0;
        int n = _mm_cvttsd_si32(_mm_load_sd(&f));
        n = table[n];

        IZ_UINT targetLevel = n / step;
#endif

        //targetLevel = IZ_MIN(targetLevel, level - 1);

        m_octree.getMortonNumberByLevel(
            mortonNumber0,
            obj0.pos,
            targetLevel0);
        m_octree.getMortonNumberByLevel(
            mortonNumber1,
            obj1.pos,
            targetLevel1);
        m_octree.getMortonNumberByLevel(
            mortonNumber2,
            obj2.pos,
            targetLevel2);
        m_octree.getMortonNumberByLevel(
            mortonNumber3,
            obj3.pos,
            targetLevel3);

        auto node0 = m_octree.getNode(mortonNumber0);
        IZ_ASSERT(node0->isContain(obj0));

        auto node1 = m_octree.getNode(mortonNumber1);
        IZ_ASSERT(node1->isContain(obj1));

        auto node2 = m_octree.getNode(mortonNumber2);
        IZ_ASSERT(node2->isContain(obj2));

        auto node3 = m_octree.getNode(mortonNumber3);
        IZ_ASSERT(node3->isContain(obj3));

        node0->add(obj0);
        node1->add(obj1);
        node2->add(obj2);
        node3->add(obj3);

        //m_acceptedNum += 4;
        //points += 4;
        //levels += 4;
        loopCnt -= 4;
    }

    while (loopCnt > 0)
    {
        auto idx0 = loopCnt - 1;

        const auto& obj0 = points[idx0];

#ifdef USE_THREAD_RAND
        IZ_UINT targetLevel0 = levels[idx0];
#else
        double f = izanagi::math::CMathRand::GetRandFloat() * 100.0;
        int n = _mm_cvttsd_si32(_mm_load_sd(&f));
        n = table[n];

        IZ_UINT targetLevel = n / step;
#endif

        //targetLevel = IZ_MIN(targetLevel, level - 1);

        m_octree.getMortonNumberByLevel(
            mortonNumber0,
            obj0.pos,
            targetLevel0);

#if 0
        auto idx = m_octree.getIndex(mortonNumber);

        auto node = m_octree.getNode(idx, IZ_TRUE);
#else
        auto node0 = m_octree.getNode(mortonNumber0);
#endif

        IZ_ASSERT(node0->isContain(obj0));

        node0->add(obj0);

        //m_acceptedNum++;
        //points++;
        //levels++;
        loopCnt--;
    }

#ifdef USE_THREAD_RAND
    if (runRand) {
        m_rand.set();
    }
#endif
}

void Writer::procRand()
{
#ifdef USE_THREAD_RAND
    auto level = m_octree.getMaxLevel();

    auto step = 100 / level;
    step++;

    for (uint32_t i = 0; i < COUNTOF(m_levels); i++) {
        double f = izanagi::math::CMathRand::GetRandFloat() * 100.0;
        int n = _mm_cvttsd_si32(_mm_load_sd(&f));

        //m_levels[i] = table[n] / step;
        m_levels[i] = table[n];
    }
#endif
}

void Writer::flush(izanagi::threadmodel::CThreadPool& threadPool)
{
#ifdef USE_THREAD_STORE
    m_store.wait();
#else
    izanagi::threadmodel::CParallel::waitFor(m_storeTasks, COUNTOF(m_storeTasks));
#endif

#ifdef USE_THREAD_FLUSH
    m_flush.wait(true);

    Node::CurIdx = 1 - Node::CurIdx;

    m_flush.set();
#else
    izanagi::threadmodel::CParallel::waitFor(m_flushTasks, COUNTOF(m_flushTasks));

    auto nodes = m_octree.getNodes();
    auto num = m_octree.getNodeCount();

    Node::CurIdx = 1 - Node::CurIdx;
    
    izanagi::threadmodel::CParallel::For(
        threadPool,
        m_flushTasks,
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

void Writer::flush()
{
    Node::CurIdx = 1 - Node::CurIdx;

    auto nodes = m_octree.getNodes();
    auto num = m_octree.getNodeCount();

    for (uint32_t i = 0; i < num; i++) {
        auto node = nodes[i];

        if (node) {
            node->flush();
        }
    }
}

void Writer::procFlush()
{
    auto list = m_octree.getNodes();
    auto num = m_octree.getNodeCount();

#if 0
    for (IZ_UINT i = 0; i < num; i++) {
        Node* node = list[i];
        if (node) {
            node->flush();
        }
    }
#else
    int loopCnt = num;

    while (loopCnt >= 4)
    {
        Node* node0 = list[0];
        Node* node1 = list[1];
        Node* node2 = list[2];
        Node* node3 = list[3];

        if (node0) {
            node0->flush();
        }
        if (node1) {
            node1->flush();
        }
        if (node2) {
            node2->flush();
        }
        if (node3) {
            node3->flush();
        }

        list += 4;
        loopCnt -= 4;
    }

    while (loopCnt > 0) {
        Node* node0 = list[0];
        if (node0) {
            node0->flush();
        }

        list++;
        loopCnt--;
    }
#endif
}

void Writer::storeDirectly(izanagi::threadmodel::CThreadPool& threadPool)
{
#ifdef USE_THREAD_STORE
    auto num = m_registeredNum;

    if (num == 0) {
        return;
    }

    m_store.wait();

#ifdef USE_THREAD_RAND
    m_rand.wait();
#endif

    m_temporary = m_objects[m_curIdx];

    m_willStoreNum = m_registeredNum;
    m_registeredNum = 0;

    m_curIdx = 1 - m_curIdx;

    procStore(false);
#else
    store(threadPool);
    izanagi::threadmodel::CParallel::waitFor(m_storeTasks, COUNTOF(m_storeTasks));
#endif
}

void Writer::flushDirectly(izanagi::threadmodel::CThreadPool& threadPool)
{
#ifdef USE_THREAD_FLUSH
    m_flush.wait();

    Node::CurIdx = 1 - Node::CurIdx;

    procFlush();
#else
    flush(threadPool);
    izanagi::threadmodel::CParallel::waitFor(m_flushTasks, COUNTOF(m_flushTasks));
#endif
}

void Writer::close(izanagi::threadmodel::CThreadPool& threadPool)
{
#ifdef USE_THREAD_STORE
    m_store.wait();
#else
    izanagi::threadmodel::CParallel::waitFor(m_storeTasks, COUNTOF(m_storeTasks));
#endif

#ifdef USE_THREAD_FLUSH
    m_flush.wait();
#else
    izanagi::threadmodel::CParallel::waitFor(m_flushTasks, COUNTOF(m_flushTasks));
#endif

    auto list = m_octree.getNodes();
    auto num = m_octree.getNodeCount();

#if 1
    for (IZ_UINT i = 0; i < num; i++) {
        Node* node = list[i];
        if (node) {
            node->close();
        }
    }
#else
    izanagi::threadmodel::CParallel::For(
        threadPool,
        m_flushTasks,
        0, num,
        [&](IZ_INT idx) {
        Node* node = list[idx];
        if (node) {
            node->close();
        }
    });
    izanagi::threadmodel::CParallel::waitFor(m_flushTasks, COUNTOF(m_flushTasks));
#endif
}

void Writer::merge(
    Writer* writer[],
    uint32_t num,
    izanagi::threadmodel::CThreadPool& threadPool)
{
    auto curIdx = Node::CurIdx.load();

    auto nodeNum = m_octree.getNodeCount();

    auto src = m_octree.getNodes();

    Node** dst[4] = { nullptr };

    for (uint32_t i = 0; i < num; i++) {
        dst[i] = writer[i]->m_octree.getNodes();
    }

    izanagi::threadmodel::CParallel::For(
        threadPool,
        m_flushTasks,
        0, nodeNum,
        [&](IZ_UINT idx)
    {
        Node* node = src[idx];

        Node* d0 = dst[0][idx];
        Node* d1 = dst[1][idx];
        Node* d2 = dst[2][idx];
        //Node* d3 = dst[3][idx];
        Node* d3 = nullptr;

        if (node) {
            auto s = node->m_vtx[curIdx];
            auto& pos = node->m_pos[curIdx];

            s += pos;

            if (d0) {
                auto d = d0->m_vtx[curIdx];
                auto n = d0->m_pos[curIdx];
                memcpy(s, d, sizeof(Point) * n);
                pos += n;
                s += n;
            }
            if (d1) {
                auto d = d1->m_vtx[curIdx];
                auto n = d1->m_pos[curIdx];
                memcpy(s, d, sizeof(Point) * n);
                pos += n;
                s += n;
            }
            if (d2) {
                auto d = d2->m_vtx[curIdx];
                auto n = d2->m_pos[curIdx];
                memcpy(s, d, sizeof(Point) * n);
                pos += n;
                s += n;
            }
            if (d3) {
                auto d = d3->m_vtx[curIdx];
                auto n = d3->m_pos[curIdx];
                memcpy(s, d, sizeof(Point) * n);
                pos += n;
                s += n;
            }
        }
        else if (d0 || d1 || d2 || d3) {
            node = m_octree.getNode(idx);
            bool isMerged = false;

            auto s = node->m_vtx[curIdx];
            auto& pos = node->m_pos[curIdx];

            s += pos;

            if (d0) {
                if (!isMerged) {
                    node->set(d0);
                    isMerged = true;
                }

                auto d = d0->m_vtx[curIdx];
                auto n = d0->m_pos[curIdx];
                memcpy(s, d, sizeof(Point) * n);
                pos += n;
                s += n;
            }
            if (d1) {
                if (!isMerged) {
                    node->set(d1);
                    isMerged = true;
                }

                auto d = d1->m_vtx[curIdx];
                auto n = d1->m_pos[curIdx];
                memcpy(s, d, sizeof(Point) * n);
                pos += n;
                s += n;
            }
            if (d2) {
                if (!isMerged) {
                    node->set(d2);
                    isMerged = true;
                }

                auto d = d2->m_vtx[curIdx];
                auto n = d2->m_pos[curIdx];
                memcpy(s, d, sizeof(Point) * n);
                pos += n;
                s += n;
            }
            if (d3) {
                if (!isMerged) {
                    node->set(d3);
                    isMerged = true;
                }

                auto d = d3->m_vtx[curIdx];
                auto n = d3->m_pos[curIdx];
                memcpy(s, d, sizeof(Point) * n);
                pos += n;
                s += n;
            }

            IZ_ASSERT(node->m_aabb.isValid())
        }
    });

    izanagi::threadmodel::CParallel::waitFor(
        m_flushTasks,
        COUNTOF(m_flushTasks));
}