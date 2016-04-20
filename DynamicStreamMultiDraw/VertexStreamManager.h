#if !defined(__VERTEX_STREAM_MANAGER_H__)
#define __VERTEX_STREAM_MANAGER_H__

#include "izStd.h"
#include "izSystem.h"
#include "izGraph.h"

struct DrawArraysIndirectCommand
{
    GLuint count;
    GLuint primCount;
    GLuint first;
    GLuint baseInstance;
};

class IVertexStreamInput {
protected:
    IVertexStreamInput() {}
    virtual ~IVertexStreamInput() {}

public:
    virtual IZ_BOOL needUpdate() = 0;

    virtual IZ_BOOL canWrite(IZ_UINT size) = 0;

    virtual IZ_UINT writeToBuffer(void* buffer, IZ_UINT offset) = 0;

    virtual IZ_UINT getSize() const = 0;
    virtual IZ_UINT getCount() const = 0;

    IZ_UINT getOffset() const
    {
        return m_offset;
    }

protected:
    IZ_UINT m_offset{ 0 };
};

class VertexStreamManager {
public:
    VertexStreamManager() {}
    ~VertexStreamManager() {}

public:
    void init(
        izanagi::IMemoryAllocator* allocator,
        izanagi::graph::CGraphicsDevice* device,
        IZ_UINT vtxStride,
        IZ_UINT vtxNum);

    void terminate();

    std::tuple<void*, IZ_UINT> getCommands();

    IZ_INT addInputSafely(IVertexStreamInput* input);

    void beginAddInput();
    void endAddInput();
    IZ_INT addInput(IVertexStreamInput* input);

    IZ_BOOL removeInput(IVertexStreamInput* input);
    IZ_BOOL removeInputByIdx(IZ_UINT idx);

    // For debug.
    void notifyUpdateForcibly();

    izanagi::graph::CVertexBuffer* getVB()
    {
        return m_vbDynamicStream;
    }

private:
    void procThread();

    struct EmptyInfo {
        IZ_UINT offset{ 0 };
        IZ_UINT size{ 0 };

        EmptyInfo(IZ_UINT _offset, IZ_UINT _size) : offset(_offset), size(_size) {}

        bool operator==(const EmptyInfo& rhs)
        {
            return (offset == rhs.offset && size == rhs.size);
        }
    };

    IVertexStreamInput* getUnprocessedInput();

    void updateEmptyInfo(EmptyInfo& info, IZ_UINT wroteSize);

    void addEmptyInfo(IZ_UINT offset, IZ_UINT size);

    void updateCommand(
        IZ_UINT count, 
        IZ_UINT offsetByte);

    void removeCommand(IZ_UINT offsetByte);

    template <typename _T>
    static IZ_INT add(_T& value, std::vector<_T>& list, std::function<void()> func);

    template <typename _T>
    static IZ_BOOL remove(_T& value, std::vector<_T>& list, std::function<void()> func);

    template <typename _T>
    static _T removeByIdx(IZ_UINT idx, std::vector<_T>& list, std::function<void()> func);

private:
    static const IZ_UINT NUM = 4;

    std::atomic<IZ_UINT> m_writingListIdx{ 0 };
    std::atomic<IZ_UINT> m_drawingListIdx{ 0 };
    std::atomic<bool> m_needChangeCmd{ false };
    izanagi::sys::CSpinLock m_lockerCmd;
    std::vector<DrawArraysIndirectCommand> m_comands[NUM];
    
    std::atomic<uint32_t> m_curIdx{ 0 };

    GLuint m_glVB{ 0 };
    GLuint m_bufferSize{ 0 };
    IZ_UINT m_vtxStride{ 0 };
    IZ_UINT m_vtxNum{ 0 };
    void* m_mappedDataPtr{ nullptr };
    izanagi::graph::CVertexBuffer* m_vbDynamicStream{ nullptr };

    izanagi::sys::CSpinLock m_lockerInputs;
    std::vector<IVertexStreamInput*> m_inputs;

    izanagi::sys::CEvent m_notifyUpdate;

    std::atomic<IZ_BOOL> m_willTerminate{ IZ_FALSE };
    std::thread m_thread;

    izanagi::sys::CSpinLock m_lockerEmptyInfo;
    std::vector<EmptyInfo> m_emptyInfo;
};

#endif    // #if !defined(__VERTEX_STREAM_MANAGER_H__)