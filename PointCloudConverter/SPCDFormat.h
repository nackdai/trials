#if !defined(__SPCD_FORMAT_H__)
#define __SPCD_FORMAT_H__

#include <stdint.h>

#define FOUR_CC(c0, c1, c2, c3)     ((((uint8_t)c0) << 24) | (((uint8_t)c1) << 16) | (((uint8_t)c2) << 8) | ((uint8_t)c3))

enum VtxFormat {
    Position = 1 << 0,
    Color = 1 << 1,
    Normal = 1 << 2,
};

struct SPCDHeader {
    uint32_t magic_number;
    uint32_t version;

    uint32_t fileSize;
    uint32_t fileSizeWithoutHeader;

    uint32_t vtxFormat;
    uint32_t vtxNum;

    uint32_t depth;
    uint32_t mortonNumber;

    float spacing;

    float aabbMin[3];
    float aabbMax[3];
};

#endif  // #if !defined(__SPCD_FORMAT_H__)
