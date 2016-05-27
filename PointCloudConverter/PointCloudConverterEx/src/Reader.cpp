#include "Reader.h"

Reader::Reader(const char* path)
{
    fopen_s(&m_fp, path, "rb");
    fread(&m_header, sizeof(m_header), 1, m_fp);
}

bool Reader::readNextPoint()
{
    if (m_isEOF) {
        return (m_pos < m_pointNum);
    }

    if (m_pos == BUFFER_NUM) {
        auto readBytes = fread(m_buffer, 1, sizeof(m_buffer), m_fp);

        if (readBytes != sizeof(m_buffer)) {
            m_isEOF = true;
            m_pointNum = readBytes / sizeof(Point);
        }

        m_pos = 0;
    }

    return true;
}

uint32_t Reader::readNextPoint(void* buffer, uint32_t size)
{
    if (m_isEOF) {
        return 0;
    }

    auto readBytes = fread(buffer, 1, size, m_fp);

    if (readBytes != size) {
        m_isEOF = true;
    }

    uint32_t pointNum = readBytes / sizeof(Point);

    return pointNum;
}


uint32_t Reader::readNextPointEx(void* buffer, uint32_t size)
{
    if (m_isEOF) {
        return 0;
    }

    auto readBytes = fread(buffer, 1, size, m_fp);

    if (readBytes != size) {
        m_isEOF = true;
    }

    uint32_t pointNum = readBytes / sizeof(Point);

    if (m_readPointNum + pointNum > m_limit) {
        m_isEOF = true;
        m_readPointNum += pointNum;
        pointNum = m_readPointNum - m_limit;
    }

    return pointNum;
}

void Reader::close()
{
    if (m_fp) {
        fclose(m_fp);
    }
}

Potree::AABB Reader::getAABB()
{
    Potree::AABB aabb;

    aabb.min.x = m_header.aabbMin[0];
    aabb.min.y = m_header.aabbMin[1];
    aabb.min.z = m_header.aabbMin[2];

    aabb.max.x = m_header.aabbMax[0];
    aabb.max.y = m_header.aabbMax[1];
    aabb.max.z = m_header.aabbMax[2];

    aabb.size = aabb.max - aabb.min;

    return std::move(aabb);
}

void Reader::seek(uint32_t idx)
{
    uint32_t offset = sizeof(Point) * idx;
    fseek(m_fp, offset, SEEK_SET);
}