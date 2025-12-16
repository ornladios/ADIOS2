#include "adios2/toolkit/filepool/FilePool.h"

PoolableFile::~PoolableFile() { m_OwningPool->Release(m_Entry); }

void PoolableFile::Read(char *buffer, size_t size, size_t start)
{
    m_Entry->m_File->Read(buffer, size, start);
}

void FilePool::Release(PoolEntry *obj)
{
    std::lock_guard<std::mutex> lockGuard(PoolMutex);
    obj->m_InUseCount--;
}

std::vector<std::shared_ptr<adios2::Transport>> FilePool::ListOfTransports()
{
    std::vector<std::shared_ptr<adios2::Transport>> Ret;
    for (auto it = m_Pool.begin(); it != m_Pool.end(); ++it)
    {
        Ret.push_back(it->second->m_File);
    }
    return Ret;
}
std::unique_ptr<PoolableFile> FilePool::Acquire(const std::string &filename)
{
    std::lock_guard<std::mutex> lockGuard(PoolMutex);
    // Use a custom deleter to return the object to the pool

    auto range = m_Pool.equal_range(filename);
    for (auto it = range.first; it != range.second; ++it)
    {
        if ((it->second->m_InUseCount == 0) || m_CanShare)
        {
            it->second->m_InUseCount++;
            return std::make_unique<PoolableFile>(this, it->second.get());
        }
    }
    // PoolEntry not found or can't be reused, we need to create, first check limit
    if (m_OpenFileCount == m_OpenFileLimit)
    {
        for (auto it = m_Pool.begin(); it != m_Pool.end(); ++it)
        {
            if (it->second->m_InUseCount == 0)
            {
                m_Pool.erase(it);
                m_OpenFileCount--;
                break;
            }
        }
        // we didn't find anything to free, can't open more, so throw
        if (m_OpenFileCount == m_OpenFileLimit)
            adios2::helper::Throw<std::runtime_error>(
                "Toolkit", "FilePool", "Acquire",
                "Tried to open more files than file limit, requested file is \"" + filename +
                    "\" limit is " + std::to_string(m_OpenFileLimit));
    }

    std::shared_ptr<adios2::Transport> file = m_Factory->OpenFileTransport(
        filename, adios2::Mode::Read, m_TransportParams, false, false, adios2::helper::CommDummy());
    auto entry = std::make_shared<PoolEntry>(filename, file);
    entry->m_InUseCount = 1;
    m_Pool.insert({filename, entry});
    std::unique_ptr<PoolableFile> ptr = std::make_unique<PoolableFile>(this, entry.get());

    if (!m_ShareTestDone)
    {
        // first time through, we'll always be creating the same transport, so see if we can reuse
        // it on read
        m_ShareTestDone = true;
        m_CanShare = file->m_ReentrantRead;
    }
    m_OpenFileCount++;
    if (m_OpenFileCount > m_MaxFileCount)
        m_MaxFileCount = m_OpenFileCount;
    return ptr;
}

void FilePool::SetParameters(const adios2::Params &params)
{
    m_TransportParams.insert(params.begin(), params.end());
}
