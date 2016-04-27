// PCLReader.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <string>
#include "proxy.h"
#include "dynamicoctree/DynamicOctree.h"
#include "Node.h"

int _tmain(int argc, _TCHAR* argv[])
{
    std::string pathIn(argv[1]);
    std::string pathOut(argv[2]);

    std::string outDir(".\\");
    int maxDepth = 3;

    DynamicOctree<Node> octree;

    auto reader = Proxy::createPointReader(pathIn);

    uint64_t pointNum = 0;

    while (reader->readNextPoint()) {
        auto point = reader->getPoint();
        pointNum++;

        DynamicOctreeObject* obj = new Object();

        octree.add(obj);

        if ((pointNum % 100000) == 0) {

        }
    }

    reader->close();
    delete reader;

	return 0;
}

