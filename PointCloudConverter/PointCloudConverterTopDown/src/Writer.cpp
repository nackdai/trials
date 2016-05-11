#include "Writer.h"

Writer::Writer(
    izanagi::IMemoryAllocator* allocator,
    const Potree::AABB& aabb)
{
    const auto& min = aabb.min;
    const auto& max = aabb.max;
    
    m_octree.initialize(
        allocator,
        izanagi::math::CVector4(min.x, min.y, min.z),
        izanagi::math::CVector4(max.x, max.y, max.z),
        3);

    m_runThread = true;

    m_objects.reserve(10000);

    m_waitMain.Set();

    m_thStore = std::thread([&] {
        while (m_runThread) {
            m_waitWorker.Wait();
            m_waitWorker.Reset();

            procStore();

            m_waitMain.Set();
        }
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
    waitForFlushing();
    
    m_waitMain.Wait();
    m_waitMain.Reset();

    m_temporary.resize(m_objects.size());

    auto num = m_objects.size();

    memcpy(
        &m_temporary[0],
        &m_objects[0],
        sizeof(Point) * num);

    m_objects.clear();

    m_waitWorker.Set();
}

void Writer::terminate()
{
    waitForFlushing();
    
    m_waitMain.Wait();

    m_runThread = false;

    m_waitWorker.Set();

    if (m_thStore.joinable()) {
        m_thStore.join();
    }
}

void Writer::procStore()
{
    auto level = m_octree.getMaxLevel();

    auto step = 100 / level;

    for (auto obj : m_temporary) {
        // TODO
        auto n = izanagi::math::CMathRand::GetRandBetween(0, 100);
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

        node->add(obj);

        m_acceptedNum++;
    }

    m_temporary.clear();
}

void Writer::flush(izanagi::threadmodel::CThreadPool& theadPool)
{
    waitForFlushing();
    
    m_waitMain.Wait();

    auto num = m_octree.getNodeCount();

    izanagi::threadmodel::CParallel::For(
        theadPool,
        0, num,
        [&](IZ_INT idx) {
        auto list = m_octree.getNodes();
        Node* node = list[idx];
        if (node) {
            node->flush();
        }
    });
}

void Writer::close(izanagi::threadmodel::CThreadPool& theadPool)
{
    waitForFlushing();
    
    m_waitMain.Wait();

    auto list = m_octree.getNodes();
    auto num = m_octree.getNodeCount();

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
}

void Writer::waitForFlushing()
{
    izanagi::threadmodel::CParallel::waitFor();
}
