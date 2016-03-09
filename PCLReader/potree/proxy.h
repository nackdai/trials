#if !defined(__POINT_CLOUD_READER_PROXY__)
#define __POINT_CLOUD_READER_PROXY__

#include <string>
#include "PointReader.h"
#include "PointAttributes.hpp"

class Proxy {
public:
    static Potree::PointReader* createPointReader(std::string path);
};

#endif