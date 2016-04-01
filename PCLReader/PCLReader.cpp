// PCLReader.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include "Writer.h"
#include "Node.h"
#include "JobQueue.h"

Potree::AABB computeAABB(const std::string& path)
{
    auto reader = Proxy::createPointReader(path);

    Potree::AABB aabb = reader->getAABB();
    aabb.update(aabb.min);
    aabb.update(aabb.max);

    return std::move(aabb);
}

int _tmain(int argc, _TCHAR* argv[])
{
    std::string pathIn(argv[1]);
    std::string pathOut(argv[2]);

    auto aabb = computeAABB(pathIn);

    std::string outDir(".\\");
    int maxDepth = 3;

    float diagonalFraction = 250.0f;
    float boudingBoxDiagonal = aabb.size.length();
    float spacing = boudingBoxDiagonal / diagonalFraction;

    auto reader = Proxy::createPointReader(pathIn);

    Writer writer(outDir, aabb, maxDepth, spacing);

    JobQueue::instance().init();

    uint64_t pointNum = 0;

    while (reader->readNextPoint()) {
        auto point = reader->getPoint();
        pointNum++;

        writer.add(point);

        if ((pointNum % 10000) == 0) {
            // 貯まりすぎないように一定数のタイミングでノードに移す.
            writer.storeToNode();
            writer.waitForStoringToNode();
        }
        else if ((pointNum % 100000) == 0) {
            writer.flush();
        }
    }

    reader->close();
    delete reader;

    writer.flush();
    writer.close();

    JobQueue::instance().terminate();

	return 0;
}

