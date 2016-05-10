#if !defined(__XYZ_READER_APP_H__)
#define __XYZ_READER_APP_H__

#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

class XyzReader {
public:
    XyzReader() {}
    ~XyzReader()
    {
        close();
    }

public:
    bool open(const std::string& path);
    void close();

    struct Vertex {
        float pos[4];
        uint8_t color[4];
    };

    bool readVtx(XyzReader::Vertex& vtx);

private:
    std::ifstream m_in;
};

#endif    // #if !defined(__XYZ_READER_APP_H__)