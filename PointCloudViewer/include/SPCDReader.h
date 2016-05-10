#if !defined(__SPCD_READER_H__)
#define __SPCD_READER_H__

#include "SPCDFormat.h"

class SPCDReader {
public:
    SPCDReader() {}
    ~SPCDReader()
    {
        close();
    }

public:
    bool open(const char* path)
    {
        if (!m_fp) {
            fopen_s(&m_fp, path, "rb");
            if (m_fp) {
                fread(&m_header, sizeof(m_header), 1, m_fp);
                return true;
            }
            else {
                return false;
            }
        }

        return true;
    }

    void close()
    {
        if (m_fp) {
            fclose(m_fp);
            m_fp = nullptr;
        }
    }

    const SPCDHeader& getHeader() const
    {
        return m_header;
    }

    void read(void** dst, uint32_t size, uint32_t cnt)
    {
        fread(*dst, size, 1, m_fp);
    }

private:
    FILE* m_fp{ nullptr };
    SPCDHeader m_header;
};

#endif    // #if !defined(__SPCD_READER_H__)