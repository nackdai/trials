#if !defined(__EXPORTER_H__)
#define __EXPORTER_H__

#include <stdint.h>
#include <thread>
#include "proxy.h"

#include "izDefs.h"

class Exporter {
public:
    Exporter() {}
    Exporter(const std::string& path)
    {
        open(path);
    }
    ~Exporter()
    {
        close();
    }

    bool open(const std::string& path)
    {
        auto err = fopen_s((FILE**)&m_fp, path.c_str(), "wb");
        VRETURN(err == 0);

        m_path = path;

        return true;
    }

    void close()
    {
        if (m_fp) {
            fclose(m_fp);
            m_fp = nullptr;
        }
    }

    bool isOpen()
    {
        return (m_fp != nullptr);
    }

    bool write(const Potree::Point& point)
    {
        bool succeeded = false;

        // TODO
        // position.

        // color.
        uint8_t colors[] = {
            point.color.x,
            point.color.y,
            point.color.z,
            255,
        };
        succeeded = write(colors, sizeof(colors));
        VRETURN(succeeded);

        return succeeded;
    }

    bool write(const void* data, uint64_t size)
    {
        IZ_ASSERT(m_fp);

        auto wroteSize = fwrite(data, size, 1, m_fp);

        return (wroteSize == size);
    }

private:
    FILE* m_fp{ nullptr };
    std::string m_path;
};

#endif
