#include "proxy.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "LASPointReader.h"
#include "PTXPointReader.h"
#include "BINPointReader.hpp"
#include "PlyPointReader.h"
#include "XYZPointReader.hpp"

#if 0
Potree::PointReader* Proxy::createPointReader(
    std::string path,
    Potree::PointAttributes pointAttributes)
#else
Potree::PointReader* Proxy::createPointReader(std::string path)
#endif
{
    Potree::PointReader* reader = nullptr;

    if (boost::iends_with(path, ".las") || boost::iends_with(path, ".laz")){
        reader = new Potree::LASPointReader(path);
    }
    else if (boost::iends_with(path, ".ptx")){
        reader = new Potree::PTXPointReader(path);
    }
    else if (boost::iends_with(path, ".ply")){
        reader = new Potree::PlyPointReader(path);
    }
    else if (boost::iends_with(path, ".xyz") || boost::iends_with(path, ".txt")){
        reader = new Potree::XYZPointReader(path, format, colorRange, intensityRange);
    }
    else if (boost::iends_with(path, ".pts")){
        vector<double> intensityRange;

        if (this->intensityRange.size() == 0){
            intensityRange.push_back(-2048);
            intensityRange.push_back(+2047);
        }

        reader = new Potree::XYZPointReader(path, format, colorRange, intensityRange);
    }
#if 0
    else if (boost::iends_with(path, ".bin")){
        reader = new Potree::BINPointReader(path, aabb, scale, pointAttributes);
    }
#endif

    return reader;

}