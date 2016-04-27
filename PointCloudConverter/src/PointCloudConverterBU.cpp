// PCLReader.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <string>
#include "proxy.h"

int _tmain(int argc, _TCHAR* argv[])
{
    std::string pathIn(argv[1]);
    std::string pathOut(argv[2]);

    std::string outDir(".\\");
    int maxDepth = 3;

    auto reader = Proxy::createPointReader(pathIn);

	return 0;
}

