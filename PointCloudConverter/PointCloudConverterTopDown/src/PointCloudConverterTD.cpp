// PCLReader.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <string>
#include "proxy.h"
#include "Writer.h"
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

#if 0
#if 0
    int limit = 80;
    int cnt = 0;

    for (cnt = 0; cnt < limit; cnt++) {
        int i = cnt;

        if (i > 0 && i % 10 == 0) {
            izanagi::_OutputDebugString("\n");
        }

        int n = i;
        n -= limit / 2;

        float theta = IZ_MATH_PI * n / float(limit);
        float s = sinf(theta);

        s = (s + 1.0f) * 0.5f;
        s *= 100.0f;
        int ns = (int)s;

        izanagi::_OutputDebugString("%d, ", ns);
    }

    izanagi::_OutputDebugString("\n");

    for (int i = cnt; i < 101; i++) {
        izanagi::_OutputDebugString("100, ");
    }
#elif 0
    int limit = 50;
    int cnt = 0;

    float a = 100.0f / (limit * limit);

    for (cnt = 0; cnt < limit; cnt++) {
        int i = cnt;

        if (i > 0 && i % 10 == 0) {
            izanagi::_OutputDebugString("\n");
        }

        int x = i;

        int y = -a * (x - limit) * (x - limit) + 100;

        izanagi::_OutputDebugString("%d, ", y);
    }

    izanagi::_OutputDebugString("\n");

    for (int i = cnt; i < 101; i++) {
        izanagi::_OutputDebugString("100, ");
    }
#else
    int limit = 50;
    int cnt = 0;

    for (cnt = 0; cnt < limit; cnt++) {
        int i = cnt;

        if (i > 0 && i % 10 == 0) {
            izanagi::_OutputDebugString("\n");
        }

        int x = i;

        float t = i / (float)limit;

        float v = (i == limit
            ? 1.0f
            : -pow(2.0f, -10.0f * t) + 1.0f);

        int y = (int)(v * 100.0f);

        izanagi::_OutputDebugString("%d, ", y);
    }

    izanagi::_OutputDebugString("\n");

    for (int i = cnt; i < 101; i++) {
        izanagi::_OutputDebugString("100, ");
    }
#endif
#endif

    Node::BasePath = option.outDir;

    // Limit max depth.
    int maxDepth = IZ_MIN(option.depth, 4);

    // TODO
    auto reader = (Potree::LASPointReader*)Proxy::createPointReader(option.inFile);

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

    aabb.min.x *= scale;
    aabb.min.y *= scale;
    aabb.min.z *= scale;

    aabb.max.x *= scale;
    aabb.max.y *= scale;
    aabb.max.z *= scale;

    aabb.size = aabb.max - aabb.min;

    aabb.makeCubic();

    Writer writer(
        &allocator,
        aabb,
        maxDepth);

    uint64_t pointNum = 0;

    bool needFlush = false;

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

    while (reader->readNextPoint()) {
        needFlush = true;

#if 0
        timer.Begin();
        auto point = reader->getPoint();
        times[Type::Get] += timer.End();
        pointNum++;

#if 0
        Point pt;
        {
            pt.pos[0] = point.position.x * scale;
            pt.pos[1] = point.position.y * scale;
            pt.pos[2] = point.position.z * scale;

            pt.color = IZ_COLOR_RGBA(point.color.x, point.color.y, point.color.z, 0xff);
        }

        writer.add(pt);
#else
        timer.Begin();
        Point& pt = writer.getNextPoint();
        {
#if 1
            pt.pos[0] = point.position.x * scale;
            pt.pos[1] = point.position.y * scale;
            pt.pos[2] = point.position.z * scale;
#else
            __m128d tmp0 = {
                point.position.x * scale,
                point.position.y * scale
            };

            auto ret = _mm_cvtpd_ps(tmp0);

            pt.pos[0] = ret.m128_f32[0];
            pt.pos[1] = ret.m128_f32[1];

            __m128d tmp1 = {
                point.position.z * scale,
                0
            };

            ret = _mm_cvtpd_ps(tmp1);

            pt.pos[2] = ret.m128_f32[0];
#endif

            pt.color = IZ_COLOR_RGBA(point.color.x, point.color.y, point.color.z, 0xff);
        }
        times[Type::Add] += timer.End();
#endif
#else
        Point& pt = writer.getNextPoint();
        reader->GetPoint<Point>(pt);
        pt.pos[0] *= scale;
        pt.pos[1] *= scale;
        pt.pos[2] *= scale;
        pt.rgba[3] = 0xff;

        pointNum++;
#endif

        if ((pointNum % STORE_LIMIT) == 0) {
            timer.Begin();
            writer.store(threadPool);
            auto time = timer.End();
            times[Type::Store] += time;
            LOG("Store - %f(ms)\n", time);
        }
        if ((pointNum % FLUSH_LIMIT) == 0) {
            timer.Begin();
            writer.flush(threadPool);
            auto time = timer.End();
            times[Type::Flush] += time;
            LOG("Flush - %f(ms)\n", time);

            needFlush = false;
        }
    }

    if (needFlush) {
        timer.Begin();
        writer.storeDirectly(threadPool);
        auto time = timer.End();
        times[Type::Store] += time;
        LOG("StoreDirectly - %f(ms)\n", time);

        timer.Begin();
        writer.flushDirectly(threadPool);
        time = timer.End();
        times[Type::Flush] += time;
        LOG("FlushDirectly - %f(ms)\n", time);
    }

    timer.Begin();
    writer.close(threadPool);
    auto time = timer.End();
    times[Type::Close] += time;
    LOG("Close - %f(ms)\n", time);

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

