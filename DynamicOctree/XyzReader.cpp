#include "XyzReader.h"
#include "izDefs.h"

bool XyzReader::open(const std::string& path)
{
    close();
    m_in.open(path.c_str(), std::ios::in);

    bool result = m_in.good();

    return result;
}

void XyzReader::close()
{
    m_in.close();
}

namespace {
    inline bool findFromFirst(std::string& lhs, const char* rhs)
    {
        return (lhs.find(rhs, 0) != std::string::npos);
    }

    inline std::string subStr(std::string& lhs, const char* rhs)
    {
        auto length = lhs.length();
        auto offset = std::string(rhs).length();
        auto substr = lhs.substr(offset, length - offset);
        return std::move(substr);
    }
}

bool XyzReader::readVtx(XyzReader::Vertex& vtx)
{
    if (m_in.eof()) {
        return false;
    }

    union Value {
        float f;
        int32_t i;
    };

    std::string line;
    getline(m_in, line);

    std::vector<std::string> values;

    std::string tmp(line);

    if (tmp.empty()) {
        return false;
    }

    size_t pos = 0;

    for (;;) {
        int length = tmp.find(" ", 0);
        auto value = tmp.substr(0, length);

        pos += length + 1;

        values.push_back(value);

        tmp = line.substr(pos, line.length() - pos);

        if (length <= 0) {
            break;
        }
    }

    vtx.pos[0] = atof(values[0].c_str());
    vtx.pos[1] = atof(values[1].c_str());
    vtx.pos[2] = atof(values[2].c_str());
    vtx.pos[3] = 1.0f;

    vtx.color[0] = atof(values[3].c_str());
    vtx.color[1] = atof(values[4].c_str());
    vtx.color[2] = atof(values[5].c_str());
    vtx.color[3] = 0xff;

    return true;
}
