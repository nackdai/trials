#if !defined(__READER_H__)
#define __READER_H__

#include "proxy.h"

#include <stdio.h>
#include "TMPFormat.h"
#include "Node.h"
#include "Config.h"

class Reader {
public:
    Reader() {}

    Reader(const char* path);

    ~Reader()
    {
        close();
    }

public:
    bool readNextPoint();

    uint32_t readNextPoint(void* buffer, uint32_t size);

    uint32_t readNextPointEx(void* buffer, uint32_t size);

    template <typename _POINT>
    void GetPoint(_POINT& p)
    {
        p = m_buffer[m_pos];
        ++m_pos;
    }

    long numPoints()
    {
        return m_header.vtxNum;
    }

    void close();

    Potree::AABB getAABB();

    void seek(uint32_t idx);

    void setLimit(uint32_t limit)
    {
        m_limit = limit;
    }

    const TMPHeader& getHeader()
    {
        return m_header;
    }

private:
    FILE* m_fp{ nullptr };
    TMPHeader m_header;

    static const uint32_t BUFFER_NUM = STORE_LIMIT;
    Point m_buffer[BUFFER_NUM];

    uint32_t m_pos{ BUFFER_NUM };
    uint32_t m_pointNum{ 0 };

    uint32_t m_limit{ 0 };
    uint32_t m_readPointNum{ 0 };

    bool m_isEOF{ false };
};

#endif    // #if !defined(__READER_H__)
