#include "Writer.h"

static int table[] = {
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
        allocator,
        izanagi::math::CVector4(min.x, min.y, min.z),
        izanagi::math::CVector4(max.x, max.y, max.z),
        depth);

#ifdef USE_THREAD_FLUSH
    m_flush.start([&] {
        procFlush();
    });
#endif

    m_store.start([&] {
        procStore();
    });

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

    izanagi::col::MortonNumber mortonNumber0;
    izanagi::col::MortonNumber mortonNumber1;
    izanagi::col::MortonNumber mortonNumber2;
    izanagi::col::MortonNumber mortonNumber3;

    while (loopCnt >= 4)
    {
        const auto& obj0 = points[0];
        const auto& obj1 = points[1];
        const auto& obj2 = points[2];
        const auto& obj3 = points[3];

#ifdef USE_THREAD_RAND
        //IZ_UINT targetLevel0 = levels[0];
        //IZ_UINT targetLevel1 = levels[1];
        //IZ_UINT targetLevel2 = levels[2];
        //IZ_UINT targetLevel3 = levels[3];

        IZ_UINT targetLevel0 = level-1;
        IZ_UINT targetLevel1 = level-1;
        IZ_UINT targetLevel2 = level-1;
        IZ_UINT targetLevel3 = level-1;
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

#if 0
        auto idx = m_octree.getIndex(mortonNumber);

        auto node = m_octree.getNode(idx, IZ_TRUE);
#else
        auto node0 = m_octree.getNodeByMortonNumber(mortonNumber0, IZ_TRUE);
        auto node1 = m_octree.getNodeByMortonNumber(mortonNumber1, IZ_TRUE);
        auto node2 = m_octree.getNodeByMortonNumber(mortonNumber2, IZ_TRUE);
        auto node3 = m_octree.getNodeByMortonNumber(mortonNumber3, IZ_TRUE);
#endif

        IZ_ASSERT(node0->isContain(obj0));
        IZ_ASSERT(node1->isContain(obj1));
        IZ_ASSERT(node2->isContain(obj2));
        IZ_ASSERT(node3->isContain(obj3));

        node0->add(obj0);
        node1->add(obj1);
        node2->add(obj2);
        node3->add(obj3);

        m_acceptedNum += 4;
        points += 4;
        levels += 4;
        loopCnt -= 4;
    }

    while (loopCnt > 0)
    {
        const auto& obj0 = points[0];

#ifdef USE_THREAD_RAND
        IZ_UINT targetLevel0 = levels[0];
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
        auto node0 = m_octree.getNodeByMortonNumber(mortonNumber0, IZ_TRUE);
#endif

        IZ_ASSERT(node0->isContain(obj0));

        node0->add(obj0);

        m_acceptedNum++;
        points++;
        levels++;
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

        m_levels[i] = table[n] / step;
    }
#endif
}

void Writer::flush(izanagi::threadmodel::CThreadPool& threadPool)
{
#ifdef USE_THREAD_FLUSH
#ifdef USE_THREAD_STORE
    m_store.wait();
#else
    izanagi::threadmodel::CParallel::waitFor(m_storeTasks, COUNTOF(m_storeTasks));
#endif

    m_flush.wait(true);

    m_flush.set();
#else

#ifdef USE_THREAD_STORE
    m_store.wait();
#else
    izanagi::threadmodel::CParallel::waitFor(m_storeTasks, COUNTOF(m_storeTasks));
#endif

    izanagi::threadmodel::CParallel::waitFor(m_flushTasks, COUNTOF(m_flushTasks));

    auto nodes = m_octree.getNodes();
    auto num = m_octree.getNodeCount();
    
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
