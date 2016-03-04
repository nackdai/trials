#include <string>
#include "izSampleKit.h"
#include "izSystem.h"
#include "izThreadModel.h"

#include <thread>

#include <lz4.h>

void Comp2Decomp(
    const char* inPath,
    const char* tmpPath,
    const char* outPath)
{
    static const int BlockSize = 4 * 1024;

    // Compress.
    {
        izanagi::CFileInputStream in;
        auto res = in.Open(inPath);
        IZ_ASSERT(res);

        izanagi::CFileOutputStream out;
        res = out.Open(tmpPath);
        IZ_ASSERT(res);

        auto outBufSize = LZ4_compressBound(BlockSize);

        void* inBuf = malloc(BlockSize);
        void* outBuf = malloc(outBufSize);

        izanagi::sys::CTimer timer;
        timer.Begin();

        for (;;) {
            auto readSize = in.Read(inBuf, 0, BlockSize);

            if (readSize == 0) {
                // end.
                break;
            }

            auto outSize = LZ4_compress_fast(
                (const char*)inBuf,
                (char*)outBuf,
                BlockSize,
                outBufSize,
                1);

            // Write out size.
            out.Write(&outSize, 0, sizeof(outSize));

            // Write out buffer.
            out.Write(outBuf, 0, outSize);
        }

        auto time = timer.End();
        IZ_PRINTF("Compress:%f[ms]\n", time);

        free(inBuf);
        free(outBuf);
    }

    // Decompress.
    {
        izanagi::CFileInputStream in;
        auto res = in.Open(tmpPath);
        IZ_ASSERT(res);

        izanagi::CFileOutputStream out;
        res = out.Open(outPath);
        IZ_ASSERT(res);

        auto compBufSize = LZ4_compressBound(BlockSize);

        void* inBuf = malloc(compBufSize);
        void* outBuf = malloc(BlockSize);

        izanagi::sys::CTimer timer;
        timer.Begin();

        for (;;) {
            // Read size.
            int size = 0;
            auto readSize = in.Read((void*)&size, 0, sizeof(size));
            if (readSize == 0) {
                break;
            }

            // Read block.
            readSize = in.Read(inBuf, 0, size);

            // decompress.
            auto decodeSize = LZ4_decompress_safe(
                (const char*)inBuf,
                (char*)outBuf,
                size,
                BlockSize);
            IZ_ASSERT(decodeSize >= 0);

            out.Write(outBuf, 0, decodeSize);
        }

        auto time = timer.End();
        IZ_PRINTF("DeCompress:%f[ms]\n", time);

        free(inBuf);
        free(outBuf);
    }
}

void CompressMT(const char* inPath)
{
    static const int BlockSize = 4 * 1024;

    izanagi::CFileInputStream in;
    auto res = in.Open(inPath);
    IZ_ASSERT(res);

    auto outBufSize = LZ4_compressBound(BlockSize);

    auto inSize = in.GetSize();

    auto blockNum = inSize / BlockSize;
    blockNum += (inSize % BlockSize == 0 ? 0 : 1);

    void* inTmp = malloc(BlockSize);
    void* outTmp = malloc(outBufSize);

    void* inBuf = malloc(inSize);

    in.Read(inBuf, 0, inSize);
    izanagi::CMemoryInputStream memIn;
    memIn.Init(inBuf, inSize);

    izanagi::CStandardMemoryAllocator allocator;
    static IZ_BYTE buf[1 * 1024 * 1024];
    allocator.Init(sizeof(buf), buf);

    izanagi::threadmodel::CThreadPool threadPool;
    threadPool.Init(&allocator, 6);

    izanagi::sys::CTimer timer;
    timer.Begin();

#if 0
    for (int i = 0; i <  blockNum; i++) {
        auto readSize = memIn.Read(inTmp, 0, BlockSize);

        auto outSize = LZ4_compress_fast(
            (const char*)inTmp,
            (char*)outTmp,
            BlockSize,
            outBufSize,
            1);
    }
#else
    izanagi::sys::CSpinLock spinLock;

    izanagi::threadmodel::CParallel::For(
        threadPool,
        &allocator,
        0, blockNum,
        [&](IZ_UINT i) {
        spinLock.lock();
        auto readSize = memIn.Read(inTmp, 0, BlockSize);
        spinLock.unlock();

        auto outSize = LZ4_compress_fast(
            (const char*)inTmp,
            (char*)outTmp,
            BlockSize,
            outBufSize,
            10);
    });
#endif

    auto time = timer.End();
    IZ_PRINTF("CompressMT:%f[ms]\n", time);

    free(inBuf);

    free(inTmp);
    free(outTmp);

    threadPool.Terminate();
}

IzMain(0, 0)
{
    IZ_ASSERT(argc > 1);

    std::string in(argv[1]);

    std::string tmp(argv[1]);
    tmp += ".tmp";

    std::string out(argv[1]);
    out += ".out";

    Comp2Decomp(in.c_str(), tmp.c_str(), out.c_str());

    CompressMT(in.c_str());

    return 0;
}
