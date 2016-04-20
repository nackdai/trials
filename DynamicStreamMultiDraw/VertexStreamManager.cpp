#include <algorithm>
#include <iterator>
#include "VertexStreamManager.h"

void VertexStreamManager::init(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    IZ_UINT vtxStride,
    IZ_UINT vtxNum)
{
    m_vbDynamicStream = device->CreateVertexBuffer(
        vtxStride, // �`��̍ۂɎg����̂ŁA�������l��ݒ肵�Ă���.
        0,         // glBufferStorage �Ŋm�ۂ����̂ŁA���̒l�̓[���ɂ��Ă���.
        izanagi::graph::E_GRAPH_RSC_USAGE_DYNAMIC);

    CALL_GL_API(::glGenBuffers(1, &m_glVB));

    m_vbDynamicStream->overrideNativeResource(&m_glVB);

    CALL_GL_API(::glBindBuffer(GL_ARRAY_BUFFER, m_glVB));

    const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    m_bufferSize = vtxStride * vtxNum;

    m_vtxStride = vtxStride;
    m_vtxNum = vtxNum;

    CALL_GL_API(::glBufferStorage(
        GL_ARRAY_BUFFER,
        m_bufferSize,
        NULL,
        flags));

    m_mappedDataPtr = ::glMapBufferRange(GL_ARRAY_BUFFER, 0, m_bufferSize, flags);
    IZ_ASSERT(m_mappedDataPtr);

    m_emptyInfo.push_back(EmptyInfo(0, m_bufferSize));

    m_thread = std::thread([&] {
        procThread();
    });
}

void VertexStreamManager::procThread()
{
    while (!m_willTerminate) {
        m_notifyUpdate.Wait();
        m_notifyUpdate.Reset();

        if (m_willTerminate) {
            break;
        }

        izanagi::sys::Lock lockerInputs(m_lockerInputs);
        izanagi::sys::Lock lockerEmptyInfo(m_lockerEmptyInfo);

        for (auto input : m_inputs) {
            if (input->needUpdate()) {
                for (auto& info : m_emptyInfo) {
                    if (input->canWrite(info.size)) {
                        auto writeSize = input->getSize();

                        updateEmptyInfo(info, writeSize);

                        writeSize = input->writeToBuffer(m_mappedDataPtr, info.offset);

                        auto count = input->getCount();

                        updateCommand(count, info.offset);

                        break;
                    }
                }
            }

            izanagi::sys::CThread::Sleep(0);
        }
    }
}

IVertexStreamInput* VertexStreamManager::getUnprocessedInput()
{
    // NOTE
    // �O�Ń��b�N���ăX���b�h�Z�[�t�ɂ��Ă�������.

    for (auto input : m_inputs) {
        if (input->needUpdate()) {
            return input;
        }
    }

    return nullptr;
}

void VertexStreamManager::updateEmptyInfo(EmptyInfo& info, IZ_UINT wroteSize)
{
    // NOTE
    // �O�Ń��b�N���ăX���b�h�Z�[�t�ɂ��Ă�������.

    if (info.size >= wroteSize) {
        info.size -= wroteSize;
        if (info.size == 0) {
            remove<EmptyInfo>(info, m_emptyInfo, []{});
        }
        else {
            info.offset += wroteSize;
        }
    }
}

void VertexStreamManager::addEmptyInfo(IZ_UINT offset, IZ_UINT size)
{
    izanagi::sys::Lock lockerEmptyInfo(m_lockerEmptyInfo);

    EmptyInfo emptyInfo(offset, size);

    EmptyInfo* parent = nullptr;

    for (auto& info : m_emptyInfo) {
        if (info.offset + info.size == offset) {
            parent = &info;
            break;
        }
    }

    if (parent) {
        parent->size += size;
    }
}

void VertexStreamManager::updateCommand(
    IZ_UINT count,
    IZ_UINT offsetByte)
{
    izanagi::sys::Lock locker(m_lockerCmd);

    DrawArraysIndirectCommand command;
    command.count = count;
    command.first = offsetByte / m_vtxStride;
    command.primCount = 1;
    command.baseInstance = 0;

    m_comands[m_writingListIdx].push_back(command);

    m_needChangeCmd = true;
}

void VertexStreamManager::removeCommand(IZ_UINT offsetByte)
{
    izanagi::sys::Lock locker(m_lockerCmd);

    auto list = m_comands[m_writingListIdx];

    for (auto it = list.begin(); it != list.end(); it++) {
        auto& cmd = *it;

        auto offset = offsetByte / m_vtxStride;
        if (cmd.first == offset) {
            list.erase(it);
            m_needChangeCmd = true;
            break;
        }
    }
}

void VertexStreamManager::terminate()
{
    m_willTerminate = IZ_TRUE;
    m_notifyUpdate.Set();
    if (m_thread.joinable()) {
        m_thread.join();
    }

    m_inputs.clear();

    for (IZ_UINT i = 0; i < COUNTOF(m_comands); i++) {
        m_comands[i].clear();
    }

    m_vbDynamicStream->overrideNativeResource(nullptr);
    CALL_GL_API(::glDeleteBuffers(1, &m_glVB));

    SAFE_RELEASE(m_vbDynamicStream);
}

std::tuple<void*, IZ_UINT> VertexStreamManager::getCommands()
{
    if (m_needChangeCmd) {
        izanagi::sys::CTimer timer;
        timer.Begin();

        izanagi::sys::Lock locker(m_lockerCmd);

        m_drawingListIdx = m_writingListIdx.load();

        IZ_UINT num = m_comands[m_drawingListIdx].size();

        if (num == 0) {
            std::tuple<void*, IZ_UINT> ret(nullptr, 0);
            return ret;
        }

        std::tuple<void*, IZ_UINT> ret(
            (void*)&m_comands[m_drawingListIdx][0],
            num);

        m_writingListIdx++;
        if (m_writingListIdx >= NUM) {
            m_writingListIdx = 0;
        }

        auto time0 = timer.End();
        timer.Begin();

#if 0
        std::copy(
            m_comands[m_drawingListIdx].begin(),
            m_comands[m_drawingListIdx].end(),
            std::back_inserter(m_comands[m_writingListIdx]));
#else
        m_comands[m_writingListIdx].resize(num);
        memcpy(
            &m_comands[m_writingListIdx][0],
            &m_comands[m_drawingListIdx][0],
            sizeof(DrawArraysIndirectCommand) * num);
#endif

        m_needChangeCmd = false;

        auto time1 = timer.End();
        IZ_PRINTF("   ---- [%f] [%f]\n", time0, time1);

        return ret;
    }
    else {
        izanagi::sys::Lock locker(m_lockerCmd);

        IZ_UINT num = m_comands[m_drawingListIdx].size();

        if (num == 0) {
            std::tuple<void*, IZ_UINT> ret(nullptr, 0);
            return ret;
        }

        std::tuple<void*, IZ_UINT> ret(
            (void*)&m_comands[m_drawingListIdx][0],
            num);

        return ret;
    }
}

IZ_INT VertexStreamManager::addInputSafely(IVertexStreamInput* input)
{
    izanagi::sys::Lock locker(m_lockerInputs);

    return add<IVertexStreamInput*>(
        input, 
        m_inputs, 
        [&] {
        m_notifyUpdate.Set();
    });
}

void VertexStreamManager::beginAddInput()
{
    m_lockerInputs.lock();
}

void VertexStreamManager::endAddInput()
{
    m_notifyUpdate.Set();
    m_lockerInputs.unlock();
}

IZ_INT VertexStreamManager::addInput(IVertexStreamInput* input)
{
    return add<IVertexStreamInput*>(
        input,
        m_inputs,
        [&] {});
}

IZ_BOOL VertexStreamManager::removeInput(IVertexStreamInput* input)
{
    izanagi::sys::Lock locker(m_lockerInputs);

    auto result = remove<IVertexStreamInput*>(
        input,
        m_inputs,
        [&] {
        m_notifyUpdate.Set();
    });

    if (result) {
        addEmptyInfo(input->getOffset(), input->getSize());
        removeCommand(input->getOffset());
    }

    return result;
}

IZ_BOOL VertexStreamManager::removeInputByIdx(IZ_UINT idx)
{
    izanagi::sys::Lock locker(m_lockerInputs);

    auto input = removeByIdx<IVertexStreamInput*>(
        idx,
        m_inputs,
        [&] {
        m_notifyUpdate.Set();
    });

    if (input) {
        addEmptyInfo(input->getOffset(), input->getSize());
        removeCommand(input->getOffset());
    }

    return (input != nullptr);
}


template <typename _T>
IZ_INT VertexStreamManager::add(_T& value, std::vector<_T>& list, std::function<void()> func)
{
    IZ_INT idx = -1;

    auto found = std::find(list.begin(), list.end(), value);

    if (found == list.end()) {
        idx = list.size();
        list.push_back(value);

        func();
    }

    return idx;
}

template <typename _T>
IZ_BOOL VertexStreamManager::remove(_T& value, std::vector<_T>& list, std::function<void()> func)
{
    auto found = std::find(list.begin(), list.end(), value);

    if (found != list.end()) {
        list.erase(found);
        func();
        return IZ_TRUE;
    }

    return IZ_FALSE;
}

template <typename _T>
_T VertexStreamManager::removeByIdx(IZ_UINT idx, std::vector<_T>& list, std::function<void()> func)
{
    VRETURN(idx < list.size());

    auto it = list.begin();
    it += idx;

    auto ret = *it;

    list.erase(it);
    func();

    return ret;
}

// For debug.
void VertexStreamManager::notifyUpdateForcibly()
{
    izanagi::sys::Lock lockerCmd(m_lockerCmd);
    izanagi::sys::Lock lockerEmptyInfo(m_lockerEmptyInfo);

    m_comands[m_writingListIdx].clear();

    m_emptyInfo.clear();
    m_emptyInfo.push_back(EmptyInfo(0, m_bufferSize));

    m_needChangeCmd = true;

    m_notifyUpdate.Set();
}