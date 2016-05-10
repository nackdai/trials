// PCLReader.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <string>
#include "proxy.h"
#include "Writer.h"

static const uint32_t STORE_LIMIT = 5000;
static const uint32_t FLUSH_LIMIT = 10000;

int _tmain(int argc, _TCHAR* argv[])
{
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

    Writer writer;

    uint64_t pointNum = 0;

    bool needFlush = false;

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
            writer.store();
        }
        if ((pointNum % FLUSH_LIMIT) == 0) {
            writer.flush(theadPool);
            needFlush = false;
        }
    }

    if (needFlush) {
        writer.flush(theadPool);
    }

    writer.close(theadPool);
    writer.terminate();

    reader->close();
    delete reader;

    theadPool.WaitEmpty();
    theadPool.Terminate();

	return 0;
}

