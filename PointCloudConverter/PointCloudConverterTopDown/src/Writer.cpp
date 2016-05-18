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

    auto loopCnt = m_willStoreNum;

    izanagi::col::MortonNumber mortonNumber;

    for (uint32_t i = 0; i < loopCnt; i++)
    //for (int32_t i = loopCnt; i--;)
    {
        const auto& obj = points[i];

#ifdef USE_THREAD_RAND
        IZ_UINT targetLevel = m_levels[i];
#else
        double f = izanagi::math::CMathRand::GetRandFloat() * 100.0;
        int n = _mm_cvttsd_si32(_mm_load_sd(&f));
        n = table[n];

        IZ_UINT targetLevel = n / step;
#endif

        //targetLevel = IZ_MIN(targetLevel, level - 1);

        m_octree.getMortonNumberByLevel(
            mortonNumber,
            //izanagi::math::CVector4(obj.pos[0], obj.pos[1], obj.pos[2]),
            obj.pos,
            targetLevel);

        auto idx = m_octree.getIndex(mortonNumber);

        auto node = m_octree.getNode(idx, IZ_TRUE);
        IZ_ASSERT(node->isContain(obj));

        node->add(obj);

        m_acceptedNum++;
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
    m_flush.wait(true);

#ifdef USE_THREAD_STORE
    m_store.wait();
#else
    izanagi::threadmodel::CParallel::waitFor(m_storeTasks, COUNTOF(m_storeTasks));
#endif

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

    for (IZ_UINT i = 0; i < num; i++) {
        Node* node = list[i];
        if (node) {
            node->flush();
        }
    }
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

#if 0
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
