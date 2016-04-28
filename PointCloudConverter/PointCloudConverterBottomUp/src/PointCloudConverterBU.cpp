// PCLReader.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <string>
#include "proxy.h"
#include "Writer.h"

int _tmain(int argc, _TCHAR* argv[])
{
    std::string pathIn(argv[1]);
    std::string pathOut(argv[2]);

    std::string outDir(".\\");
    int maxDepth = 3;

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

        Object* obj = new Object();
        {
            obj->pos[0] = point.position.x;
            obj->pos[1] = point.position.y;
            obj->pos[2] = point.position.z;

            obj->color = IZ_COLOR_RGBA(point.color.x, point.color.y, point.color.z, 0xff);
        }

        writer.add(obj);

        if ((pointNum % 100000) == 0) {
            writer.store();
        }
        else if ((pointNum % 1000000) == 0) {
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

