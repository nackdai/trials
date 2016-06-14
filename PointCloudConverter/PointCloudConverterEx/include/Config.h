#if !defined(__CONFIG_H__)
#define __CONFIG_H__

#include <stdint.h>
#include "izDefs.h"

#define MAJOR_VERSION   (0)
#define MINOR_VERSION   (0)
#define REVISION        (1)

#if 1
#define LOG(fmt, ...) izanagi::_OutputDebugString(fmt, __VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

//static const uint32_t STORE_LIMIT = 10000;
static const uint32_t STORE_LIMIT = 10000;
//static const uint32_t FLUSH_LIMIT = 1000000;
static const uint32_t FLUSH_LIMIT = 200000;

#define MAIN_THREAD_WRITER

#endif    // #if !defined(__CONFIG_H__)