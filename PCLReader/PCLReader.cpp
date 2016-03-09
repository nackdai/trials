// PCLReader.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include "proxy.h"

int _tmain(int argc, _TCHAR* argv[])
{
    std::string path(argv[1]);

    auto reader = Proxy::createPointReader(path);



    reader->close();
    delete reader;

	return 0;
}

