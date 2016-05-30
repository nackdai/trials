// PCLReader.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <string>
#include "proxy.h"
#include "Writer.h"
#include "Reader.h"
#include "Config.h"

void dispUsage()
{
    printf("PointCloudConverter %d.%d.%d\n", MAJOR_VERSION, MINOR_VERSION, REVISION);
    printf("Usage *****\n");
    printf(" PointCloudConverterTD.exe [options] [input]\n");
    printf("  -o [dir]   : output directory.\n");
    printf("  -s [scale] : scale.\n");
    printf("  -d [depth] : octree depth.\n");
}

struct SOption {
    std::string outDir;
    float scale{ 1.0f };
    uint32_t depth{ 1 };
    std::string inFile;
};

class Option : public SOption {
public:
    Option() {}
    ~Option() {}

public:
    bool parse(int argc, _TCHAR* argv[]);
};

#define GET_ARG(idx, argc, argv)\
    idx++; if (idx >= argc) { return false; }\
    std::string arg(argv[idx]);\
    if (arg.empty()) { return false; }

bool Option::parse(int argc, _TCHAR* argv[])
{
    if (argc <= 1) {
        return false;
    }

    for (int i = 1; i < argc - 1; i++) {
        std::string opt(argv[i]);

        if (opt == "-o") {
            GET_ARG(i, argc, argv);
            outDir = arg;

            if (outDir.back() != '\\' || outDir.back() != '/') {
                outDir += "\\";
            }
        }
        else if (opt == "-s") {
            GET_ARG(i, argc, argv);
            scale = atof(arg.c_str());
        }
        else if (opt == "-d") {
            GET_ARG(i, argc, argv);
            depth = atoi(arg.c_str());
        }
        else {
            return false;
        }
    }

    inFile = argv[argc - 1];

    return true;
}

#ifdef MAIN_THREAD_WRITER
int _tmain(int argc, _TCHAR* argv[])
{
    Option option;
    if (!option.parse(argc, argv)) {
        dispUsage();
        return 1;
    }

    SYSTEMTIME st;
    GetSystemTime(&st);

    izanagi::math::CMathRand::Init(st.wMilliseconds);

    Node::BasePath = option.outDir;

    // Limit max depth.
    int maxDepth = IZ_MIN(option.depth, 4);

    // TODO
    //auto reader = (Potree::LASPointReader*)Proxy::createPointReader(option.inFile);
    auto reader = new Reader(option.inFile.c_str());

    if (!reader) {
        // TODO
        return 1;
    }

    izanagi::CSimpleMemoryAllocator allocator;

    izanagi::threadmodel::CThreadPool threadPool;
    threadPool.Init(&allocator, 4);

    auto aabb = reader->getAABB();
    IZ_ASSERT(aabb.isValid());

    // If specified scale is zero, compute scale.
    float scale = option.scale;

    if (scale <= 0.0f) {
        auto len = aabb.size.length();

        if (len > 100000.0f) {
            scale = 0.01f;
        }
        else if (len > 10000.0f) {
            scale = 0.1f;
        }
        else if (len < 10.0f) {
            scale = 100.0f;
        }
        else {
            scale = 1.0f;
        }
    }

#if 0
    aabb.min.x *= scale;
    aabb.min.y *= scale;
    aabb.min.z *= scale;

    aabb.max.x *= scale;
    aabb.max.y *= scale;
    aabb.max.z *= scale;
#else
    Node::Scale = scale;
#endif

    aabb.size = aabb.max - aabb.min;

    aabb.makeCubic();

    Writer writer(
        &allocator,
        aabb,
        maxDepth);

    uint64_t pointNum = 0;
    uint64_t storeNum = 0;
    uint64_t flushNum = 0;

    enum Type {
        Read,
        Store,
        Flush,
        Get,
        Close,
        Add,
        Num,
    };

    IZ_FLOAT times[Type::Num] = { 0.0f };

    izanagi::sys::CTimer timerEntire;
    timerEntire.Begin();

    izanagi::sys::CTimer timer;

#if 0
    while (reader->readNextPoint()) {
        Point& pt = writer.getNextPoint();
        reader->GetPoint<Point>(pt);

        pointNum++;
        storeNum++;
        flushNum++;
#else
    while (true) {
        timer.Begin();
        auto buffer = writer.getBuffer();
        auto num = reader->readNextPoint(buffer, sizeof(Point) * STORE_LIMIT);
        auto t = timer.End();
        times[Type::Get] += t;

        if (num == 0) {
            break;
        }

        writer.m_registeredNum += num;

        pointNum += num;
        storeNum += num;
        flushNum += num;
#endif

        if (storeNum == STORE_LIMIT) {
            timer.Begin();
            writer.store(threadPool);
            auto time = timer.End();
            times[Type::Store] += time;
            //LOG("Store - %f(ms)\n", time);

            storeNum = 0;
        }
        if (flushNum == FLUSH_LIMIT) {
            printf("%d\n", pointNum);

            timer.Begin();
            writer.flush(threadPool);
            auto time = timer.End();
            times[Type::Flush] += time;
            //LOG("Flush - %f(ms)\n", time);

            flushNum = 0;
        }
    }

    if (storeNum) {
        timer.Begin();
        writer.storeDirectly(threadPool);
        auto time = timer.End();
        times[Type::Store] += time;
        LOG("StoreDirectly - %f(ms)\n", time);
    }

    if (flushNum) {
        timer.Begin();
        writer.flushDirectly(threadPool);
        auto time = timer.End();
        times[Type::Flush] += time;
        LOG("FlushDirectly - %f(ms)\n", time);
    }

    timer.Begin();
    writer.close(threadPool);
    auto time = timer.End();
    times[Type::Close] += time;
    //LOG("Close - %f(ms)\n", time);

    writer.terminate();

    auto timeEntire = timerEntire.End();
    izanagi::_OutputDebugString("Time * %f(ms)\n", timeEntire);

    reader->close();
    delete reader;

    threadPool.WaitEmpty();
    threadPool.Terminate();

    izanagi::_OutputDebugString("FlushedNum [%d]\n", Node::FlushedNum);

    izanagi::_OutputDebugString("Read - %f(ms)\n", times[Type::Read]);
    izanagi::_OutputDebugString("Get - %f(ms)\n", times[Type::Get]);
    izanagi::_OutputDebugString("Add - %f(ms)\n", times[Type::Add]);
    izanagi::_OutputDebugString("Store - %f(ms)\n", times[Type::Store]);
    izanagi::_OutputDebugString("Flush - %f(ms)\n", times[Type::Flush]);
    izanagi::_OutputDebugString("Close - %f(ms)\n", times[Type::Close]);

    auto nodes = writer.m_octree.getNodes();
    auto cnt = writer.m_octree.getNodeCount();

    float setvaluetime = 0.0f;

    for (int i = 0; i < cnt; i++) {
        auto node = nodes[i];

        if (node) {
            setvaluetime += node->m_setvaluetime;
        }
    }

    izanagi::_OutputDebugString("SetValue - %f(ms)\n", setvaluetime);

	return 0;
}
#else
int _tmain(int argc, _TCHAR* argv[])
{
    Option option;
    if (!option.parse(argc, argv)) {
        dispUsage();
        return 1;
    }

    SYSTEMTIME st;
    GetSystemTime(&st);

    izanagi::math::CMathRand::Init(st.wMilliseconds);

    Node::BasePath = option.outDir;

    // Limit max depth.
    int maxDepth = IZ_MIN(option.depth, 4);

    Reader* readers[4];
    readers[0] = new Reader(option.inFile.c_str());

    auto num = readers[0]->numPoints();
    auto step = num / COUNTOF(readers);
    readers[0]->setLimit(step);
    num -= step;

    for (int i = 1; i < COUNTOF(readers); i++) {
        readers[i] = new Reader(option.inFile.c_str());
        if (i < COUNTOF(readers) - 1) {
            readers[i]->setLimit(step);
        }
        else {
            readers[i]->setLimit(num);
        }
        num -= step;
    }

    izanagi::CSimpleMemoryAllocator allocator;

    izanagi::threadmodel::CThreadPool threadPool;
    threadPool.Init(&allocator, 4);

    izanagi::threadmodel::CParallelFor parallelTasks[4];

    auto aabb = readers[0]->getAABB();
    IZ_ASSERT(aabb.isValid());

    // If specified scale is zero, compute scale.
    float scale = option.scale;

    if (scale <= 0.0f) {
        auto len = aabb.size.length();

        if (len > 100000.0f) {
            scale = 0.01f;
        }
        else if (len > 10000.0f) {
            scale = 0.1f;
        }
        else if (len < 10.0f) {
            scale = 100.0f;
        }
        else {
            scale = 1.0f;
        }
    }

    Node::Scale = scale;

    aabb.size = aabb.max - aabb.min;

    aabb.makeCubic();

    Writer* writers[4];

    for (int i = 0; i < COUNTOF(writers); i++) {
        writers[i] = new Writer(
            &allocator,
            aabb,
            maxDepth);
    }

    std::atomic<uint64_t> pointNum = 0;
    uint64_t storeNums[4] = { 0 };
    uint64_t flushNums[4] = { 0 };

    enum Type {
        Read,
        Store,
        Flush,
        Merge,
        Close,
        Add,
        Num,
    };

    IZ_FLOAT times[Type::Num] = { 0.0f };

    izanagi::sys::CTimer timerEntire;
    timerEntire.Begin();

    izanagi::sys::CTimer timer;

#if 1
    izanagi::threadmodel::CParallel::For(
        threadPool,
        parallelTasks,
        0, 4,
        [&] (IZ_UINT idx)
#else
    int idx = 0;
#endif
    {
        auto writer = writers[idx];
        auto reader = readers[idx];

        auto& storeNum = storeNums[idx];
        auto& flushNum = flushNums[idx];

        while (true) {
            auto buffer = writer->getBuffer();
            auto num = reader->readNextPointEx(buffer, sizeof(Point) * STORE_LIMIT);

            if (num == 0) {
                if (storeNum > 0) {
                    writer->store();
                    storeNum = 0;
                }
                if (flushNum > 0) {
                    izanagi::_OutputDebugString("%d\n", pointNum);
                    writer->flush();
                    flushNum = 0;
                }

                break;
            }

            writer->m_registeredNum += num;

            pointNum += num;
            storeNum += num;
            flushNum += num;

            if (flushNum == FLUSH_LIMIT) {
                izanagi::_OutputDebugString("%d\n", pointNum);
                writer->flush();
                flushNum = 0;
            }
            if (storeNum == STORE_LIMIT) {
                writer->store();
                storeNum = 0;
            }
        }
#if 1
    });
#else
    }
#endif

    izanagi::threadmodel::CParallel::waitFor(
        parallelTasks,
        COUNTOF(parallelTasks));

    timer.Begin();
    for (int i = 0; i < COUNTOF(writers); i++) {
        writers[i]->close(threadPool);
    }
    auto time = timer.End();
    times[Type::Close] += time;
    LOG("Close - %f(ms)\n", time);

    for (int i = 0; i < COUNTOF(writers); i++) {
        writers[i]->terminate();
        delete writers[i];
    }

    auto timeEntire = timerEntire.End();
    izanagi::_OutputDebugString("Time * %f(ms)\n", timeEntire);

    for (int i = 0; i < COUNTOF(readers); i++) {
        readers[i]->close();
        delete readers[i];
    }

    threadPool.WaitEmpty();
    threadPool.Terminate();

    izanagi::_OutputDebugString("FlushedNum [%d]\n", Node::FlushedNum);

    izanagi::_OutputDebugString("Read - %f(ms)\n", times[Type::Read]);
    izanagi::_OutputDebugString("Merge - %f(ms)\n", times[Type::Merge]);
    izanagi::_OutputDebugString("Add - %f(ms)\n", times[Type::Add]);
    izanagi::_OutputDebugString("Store - %f(ms)\n", times[Type::Store]);
    izanagi::_OutputDebugString("Flush - %f(ms)\n", times[Type::Flush]);
    izanagi::_OutputDebugString("Close - %f(ms)\n", times[Type::Close]);

    return 0;
}
#endif