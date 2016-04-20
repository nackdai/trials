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

// 頂点データ入力インターフェース.
class IVertexStreamInput {
protected:
    IVertexStreamInput() {}
    virtual ~IVertexStreamInput() {}

public:
    // 更新が必要かどうか.
    virtual IZ_BOOL needUpdate() = 0;

    // 書き込み可能かどうか.
    virtual IZ_BOOL canWrite(IZ_UINT size) = 0;

    // 指定バッファに書き込み.
    virtual IZ_UINT writeToBuffer(void* buffer, IZ_UINT offset) = 0;

    // 書き込みサイズを取得.
    virtual IZ_UINT getSize() const = 0;

    // 書き込みデータ個数を取得.
    virtual IZ_UINT getCount() const = 0;

    // 書き込みオフセット位置を取得.
    IZ_UINT getOffset() const
    {
        return m_offset;
    }

protected:
    IZ_UINT m_offset{ 0 };
};

// 頂点データストリームマネージャー.
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

    // 描画コマンドを取得.
    std::tuple<void*, IZ_UINT> getCommands();

    // スレッドセーフで入力データを追加.
    IZ_INT addInputSafely(IVertexStreamInput* input);

    // 入力データを追加を開始.
    void beginAddInput();

    // 入力データを追加を終了.
    void endAddInput();

    // 入力データを追加.
    IZ_INT addInput(IVertexStreamInput* input);

    // 入力データを削除.
    IZ_BOOL removeInput(IVertexStreamInput* input);

    // 指定インデックスの入力データを削除.
    IZ_BOOL removeInputByIdx(IZ_UINT idx);

    // For debug.
    void notifyUpdateForcibly();

    izanagi::graph::CVertexBuffer* getVB()
    {
        return m_vbDynamicStream;
    }

private:
    void procThread();

    // バッファの空き位置情報.
    struct EmptyInfo {
        IZ_UINT offset{ 0 };
        IZ_UINT size{ 0 };

        EmptyInfo(IZ_UINT _offset, IZ_UINT _size) : offset(_offset), size(_size) {}

        bool operator==(const EmptyInfo& rhs)
        {
            return (offset == rhs.offset && size == rhs.size);
        }
    };

    // 未処理の入力データを取得.
    IVertexStreamInput* getUnprocessedInput();

    // 空き情報を更新.
    void updateEmptyInfo(EmptyInfo& info, IZ_UINT wroteSize);

    // 空き情報を追加.
    void addEmptyInfo(IZ_UINT offset, IZ_UINT size);

    // 描画コマンドを更新.
    void updateCommand(
        IZ_UINT count, 
        IZ_UINT offsetByte);

    // 描画コマンドを削除.
    void removeCommand(IZ_UINT offsetByte);

    // リストに要素を追加.
    template <typename _T>
    static IZ_INT add(_T& value, std::vector<_T>& list, std::function<void()> func);

    // リストから要素を削除.
    template <typename _T>
    static IZ_BOOL remove(_T& value, std::vector<_T>& list, std::function<void()> func);

    // リストから指定インデックスの要素を削除.
    template <typename _T>
    static _T removeByIdx(IZ_UINT idx, std::vector<_T>& list, std::function<void()> func);

private:
    static const IZ_UINT NUM = 4;

    // 書き込みリストインデックス.
    std::atomic<IZ_UINT> m_writingListIdx{ 0 };

    // 描画参照リストインデックス.
    std::atomic<IZ_UINT> m_drawingListIdx{ 0 };

    // 描画コマンドリストの参照位置変更フラグ.
    std::atomic<bool> m_needChangeCmd{ false };

    // 描画コマンド.
    izanagi::sys::CSpinLock m_lockerCmd;
    std::vector<DrawArraysIndirectCommand> m_comands[NUM];
    
    GLuint m_glVB{ 0 };

    GLuint m_bufferSize{ 0 };
    IZ_UINT m_vtxStride{ 0 };
    IZ_UINT m_vtxNum{ 0 };

    void* m_mappedDataPtr{ nullptr };
    izanagi::graph::CVertexBuffer* m_vbDynamicStream{ nullptr };

    // 入力データ.
    izanagi::sys::CSpinLock m_lockerInputs;
    std::vector<IVertexStreamInput*> m_inputs;

    // 入力データリストの更新通知用.
    izanagi::sys::CEvent m_notifyUpdate;

    // スレッド終了フラグ.
    std::atomic<IZ_BOOL> m_willTerminate{ IZ_FALSE };

    std::thread m_thread;

    // 空き情報.
    izanagi::sys::CSpinLock m_lockerEmptyInfo;
    std::vector<EmptyInfo> m_emptyInfo;
};

#endif    // #if !defined(__VERTEX_STREAM_MANAGER_H__)