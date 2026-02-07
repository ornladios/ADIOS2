#include "adios2/toolkit/filepool/FilePool.h"
#include "adios2sys/SystemTools.hxx"

PoolableFile::~PoolableFile() { m_OwningPool->Release(m_Entry); }

void PoolableFile::Read(char *buffer, size_t size, size_t start)
{
    m_Entry->m_File->Read(buffer, size, start + m_BaseOffset);
}

size_t PoolableFile::GetSize()
{
    if (m_BaseSize != (size_t)-1)
    {
        return m_BaseSize; // TarInfo
    }
    else
    {
        return m_Entry->m_File->GetSize();
    }
}

void PoolableFile::SetParameters(const adios2::Params &params)
{
    m_Entry->m_File->SetParameters(params);
}

void PoolableFile::Close() {}

void FilePool::Release(PoolEntry *obj)
{
    std::lock_guard<std::mutex> lockGuard(PoolMutex);
    obj->m_InUseCount--;
}

void FilePool::Evict(const std::string &filename)
{
    std::lock_guard<std::mutex> lockGuard(PoolMutex);

    /* Resolve TarInfo the same way Acquire() does.  If this filename maps
       into a tar container, the pool entry is the containing file and is
       shared with other files inside the tar — do NOT evict it. */
    auto finalFileName = filename;
    if (m_TarInfoMap && m_TarInfoMap->size())
    {
        auto FilenameInTar = adios2sys::SystemTools::GetFilenameName(filename);
        auto it = m_TarInfoMap->find(FilenameInTar);
        if (it != m_TarInfoMap->end())
        {
            return; // file lives inside a tar — transport is shared, leave it alone
        }
    }

    auto range = m_Pool.equal_range(finalFileName);
    for (auto it = range.first; it != range.second;)
    {
        if (it->second->m_InUseCount == 0)
        {
            it = m_Pool.erase(it);
            m_OpenFileCount--;
        }
        else
        {
            ++it;
        }
    }
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
std::unique_ptr<PoolableFile> FilePool::Acquire(const std::string &filename, const bool skipTarInfo)
{
    std::lock_guard<std::mutex> lockGuard(PoolMutex);
    // Use a custom deleter to return the object to the pool

    auto finalFileName = filename;
    size_t offset = 0;
    size_t size = (size_t)-1;
    if (!skipTarInfo && m_TarInfoMap && m_TarInfoMap->size())
    {
        auto FilenameInTar = adios2sys::SystemTools::GetFilenameName(filename);
        auto it = m_TarInfoMap->find(FilenameInTar);
        if (it != m_TarInfoMap->end())
        {
            offset = std::get<0>(it->second);
            size = std::get<1>(it->second);
            finalFileName = filename.substr(0, filename.length() - FilenameInTar.length() - 1);
        }
    }
    auto range = m_Pool.equal_range(finalFileName);
    for (auto it = range.first; it != range.second; ++it)
    {
        if ((it->second->m_InUseCount == 0) || m_CanShare)
        {
            it->second->m_InUseCount++;
            return std::make_unique<PoolableFile>(this, it->second.get(), offset, size);
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
                "Tried to open more files than file limit, requested file is \"" + finalFileName +
                    "\" limit is " + std::to_string(m_OpenFileLimit));
    }

    std::shared_ptr<adios2::Transport> file =
        m_Factory->OpenFileTransport(finalFileName, adios2::Mode::Read, m_TransportParams, false,
                                     false, adios2::helper::CommDummy());
    auto entry = std::make_shared<PoolEntry>(finalFileName, file);
    entry->m_InUseCount = 1;
    m_Pool.insert({finalFileName, entry});
    std::unique_ptr<PoolableFile> ptr =
        std::make_unique<PoolableFile>(this, entry.get(), offset, size);

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
