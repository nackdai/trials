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
        auto buffer = writer.getBuffer();
        auto num = reader->readNextPoint(buffer, sizeof(Point) * STORE_LIMIT);

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
        //timer.Begin();
        writer.storeDirectly(threadPool);
        //auto time = timer.End();
        //times[Type::Store] += time;
        //LOG("StoreDirectly - %f(ms)\n", time);
    }

    if (flushNum) {
        //timer.Begin();
        writer.flushDirectly(threadPool);
        //auto time = timer.End();
        //times[Type::Flush] += time;
        //LOG("FlushDirectly - %f(ms)\n", time);
    }

    //timer.Begin();
    writer.close(threadPool);
    //auto time = timer.End();
    //times[Type::Close] += time;
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

	return 0;
}

