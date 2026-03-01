#include "adios2/toolkit/filepool/SharedTarFDCache.h"
#include "adios2/helper/adiosCommDummy.h"

namespace adios2
{

std::atomic<bool> SharedTarFDCache::s_Enabled{false};

void SharedTarFDCache::Enable() { s_Enabled.store(true, std::memory_order_relaxed); }

bool SharedTarFDCache::IsEnabled() { return s_Enabled.load(std::memory_order_relaxed); }

SharedTarFDCache &SharedTarFDCache::getInstance()
{
    static SharedTarFDCache instance;
    return instance;
}

std::shared_ptr<Transport> SharedTarFDCache::Acquire(const std::string &tarPath,
                                                     transportman::TransportMan *factory,
                                                     const Params &transportParams)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Cache.find(tarPath);
    if (it != m_Cache.end())
    {
        it->second.refcount++;
        return it->second.transport;
    }
    // Open a new transport for this tar file
    auto transport = factory->OpenFileTransport(tarPath, Mode::Read, transportParams, false, false,
                                                helper::CommDummy());
    Entry entry;
    entry.transport = transport;
    entry.refcount = 1;
    m_Cache.insert({tarPath, std::move(entry)});
    return transport;
}

void SharedTarFDCache::Release(const std::string &tarPath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Cache.find(tarPath);
    if (it != m_Cache.end() && it->second.refcount > 0)
    {
        it->second.refcount--;
    }
}

void SharedTarFDCache::CloseIdle()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto it = m_Cache.begin(); it != m_Cache.end();)
    {
        if (it->second.refcount == 0)
        {
            it = m_Cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

size_t SharedTarFDCache::Size()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Cache.size();
}

} // end namespace adios2
