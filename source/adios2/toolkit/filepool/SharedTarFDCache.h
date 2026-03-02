/*
 * SharedTarFDCache
 *
 * Process-wide singleton that allows multiple BP5 engines reading from
 * the same tar file to share a single file descriptor.  Inactive by
 * default — must be explicitly enabled (e.g. by the xrootd server).
 */

#ifndef ADIOS2_SHAREDTARFDCACHE_H_
#define ADIOS2_SHAREDTARFDCACHE_H_

#include "adios2/toolkit/transport/Transport.h"
#include "adios2/toolkit/transportman/TransportMan.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace adios2
{

class SharedTarFDCache
{
public:
    static SharedTarFDCache &getInstance();

    /// Enable the cache.  Called once at startup by the xrootd server.
    static void Enable();

    /// True if Enable() has been called.
    static bool IsEnabled();

    /// Return a shared Transport for the tar file at @p tarPath.
    /// If one already exists the same Transport is returned (refcount++).
    /// Otherwise it is opened via @p factory and cached.
    std::shared_ptr<Transport> Acquire(const std::string &tarPath,
                                       transportman::TransportMan *factory,
                                       const Params &transportParams);

    /// Decrement refcount for @p tarPath.  When it hits 0 the entry
    /// becomes idle but is not closed immediately.
    void Release(const std::string &tarPath);

    /// Close and remove all entries whose refcount is 0.
    void CloseIdle();

    /// Number of entries currently in the cache (for stats logging).
    size_t Size();

private:
    SharedTarFDCache() = default;
    SharedTarFDCache(const SharedTarFDCache &) = delete;
    SharedTarFDCache &operator=(const SharedTarFDCache &) = delete;

    struct Entry
    {
        std::shared_ptr<Transport> transport;
        size_t refcount = 0;
    };

    std::mutex m_Mutex;
    std::unordered_map<std::string, Entry> m_Cache;
    static std::atomic<bool> s_Enabled;
};

} // end namespace adios2

#endif /* ADIOS2_SHAREDTARFDCACHE_H_ */
