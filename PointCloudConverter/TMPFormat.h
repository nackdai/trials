#if !defined(__TMP_FORMAT_H__)
#define __TMP_FORMAT_H__

#include <stdint.h>
#include "SPCDFormat.h"

struct TMPHeader {
    uint32_t fileSize;

    uint32_t vtxFormat;
    uint32_t vtxNum;

    float aabbMin[3];
    float aabbMax[3];
};

#endif  // #if !defined(__TMP_FORMAT_H__)
