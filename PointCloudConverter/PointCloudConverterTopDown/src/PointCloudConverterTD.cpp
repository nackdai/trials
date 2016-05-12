// PCLReader.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <string>
#include "proxy.h"
#include "Writer.h"

static const uint32_t STORE_LIMIT = 3000;
static const uint32_t FLUSH_LIMIT = 10000;

int _tmain(int argc, _TCHAR* argv[])
{
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

    std::string pathIn(argv[1]);
    //std::string pathOut(argv[2]);

    std::string outDir(".\\");

    // TODO
    int maxDepth = 3;
    float scale = 100.0f;

    izanagi::CSimpleMemoryAllocator allocator;

    izanagi::threadmodel::CThreadPool theadPool;
    theadPool.Init(&allocator, 4);

    auto reader = Proxy::createPointReader(pathIn);

    auto aabb = reader->getAABB();
    IZ_ASSERT(aabb.isValid());

    aabb.min.x *= scale;
    aabb.min.y *= scale;
    aabb.min.z *= scale;

    aabb.max.x *= scale;
    aabb.max.y *= scale;
    aabb.max.z *= scale;

    aabb.size = aabb.max - aabb.min;

    Writer writer(&allocator, aabb);

    uint64_t pointNum = 0;

    bool needFlush = false;

    izanagi::sys::CTimer timer;

    while (reader->readNextPoint()) {
        needFlush = true;

        auto point = reader->getPoint();
        pointNum++;

        Point pt;
        {
            pt.pos[0] = point.position.x * scale;
            pt.pos[1] = point.position.y * scale;
            pt.pos[2] = point.position.z * scale;

            pt.color = IZ_COLOR_RGBA(point.color.x, point.color.y, point.color.z, 0xff);
        }

        writer.add(pt);

        if ((pointNum % STORE_LIMIT) == 0) {
            timer.Begin();
            writer.store();
            auto time = timer.End();
            izanagi::_OutputDebugString("Store - %f(ms)\n", time);
        }
        if ((pointNum % FLUSH_LIMIT) == 0) {
            timer.Begin();
            writer.flush(theadPool);
            auto time = timer.End();
            izanagi::_OutputDebugString("Flush - %f(ms)\n", time);

            needFlush = false;
        }
    }

    if (needFlush) {
        timer.Begin();
        writer.storeDirectly();
        auto time = timer.End();
        izanagi::_OutputDebugString("StoreDirectly - %f(ms)\n", time);

        timer.Begin();
        writer.flushDirectly();
        time = timer.End();
        izanagi::_OutputDebugString("FlushDirectly - %f(ms)\n", time);
    }

    timer.Begin();
    writer.close(theadPool);
    auto time = timer.End();
    izanagi::_OutputDebugString("Close - %f(ms)\n", time);

    writer.terminate();

    reader->close();
    delete reader;

    theadPool.WaitEmpty();
    theadPool.Terminate();

    izanagi::_OutputDebugString("FlushedNum [%d]\n", Node::FlushedNum);

	return 0;
}

