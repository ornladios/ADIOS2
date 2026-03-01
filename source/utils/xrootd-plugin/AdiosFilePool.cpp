#include "AdiosFilePool.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <sys/resource.h>
#include <sys/stat.h>
#include <thread>

namespace adios2
{

// Extract the TarInfo value from a TAB-separated key=value EngineParams string.
// Returns empty string if not found.
static std::string ExtractTarInfo(const std::string &EngineParams)
{
    std::stringstream ss(EngineParams);
    std::string entry;
    while (std::getline(ss, entry, '\t'))
    {
        if (entry.compare(0, 8, "TarInfo=") == 0)
        {
            return entry.substr(8);
        }
    }
    return std::string();
}

// Parse a TarInfo string ("filename,offset,size;filename,offset,size;...")
// to extract metadata sizes and data subfile count.
BP5CostEstimate ADIOSFilePool::ParseTarInfoCost(const std::string &tarinfo)
{
    BP5CostEstimate cost;
    cost.is_tar = true;

    std::stringstream ss(tarinfo);
    std::string entry;
    while (std::getline(ss, entry, ';'))
    {
        if (entry.empty())
        {
            continue;
        }
        // Parse "filename,offset,size"
        auto comma1 = entry.find(',');
        if (comma1 == std::string::npos)
        {
            continue;
        }
        std::string name = entry.substr(0, comma1);

        auto comma2 = entry.find(',', comma1 + 1);
        if (comma2 == std::string::npos)
        {
            continue;
        }
        size_t size = static_cast<size_t>(strtoull(entry.substr(comma2 + 1).c_str(), nullptr, 10));

        // Strip directory prefix if present (e.g. "foo.bp/md.0" -> "md.0")
        auto slash = name.rfind('/');
        if (slash != std::string::npos)
        {
            name = name.substr(slash + 1);
        }

        if (name == "md.0" || name == "mmd.0" || name == "md.idx")
        {
            cost.metadata_bytes += size;
        }
        else if (name.compare(0, 5, "data.") == 0)
        {
            cost.subfile_count++;
        }
    }

    return cost;
}

BP5CostEstimate ADIOSFilePool::ProbeBP5Directory(const std::string &path,
                                                 const std::string &EngineParams)
{
    // If EngineParams contains TarInfo, parse cost from that instead of the filesystem
    if (!EngineParams.empty())
    {
        std::string tarinfo = ExtractTarInfo(EngineParams);
        if (!tarinfo.empty())
        {
            return ParseTarInfoCost(tarinfo);
        }
    }

    BP5CostEstimate cost;
    struct stat st;

    // Stat metadata files
    std::string md0 = path + "/md.0";
    std::string mmd0 = path + "/mmd.0";
    std::string mdidx = path + "/md.idx";

    if (stat(md0.c_str(), &st) == 0)
    {
        cost.metadata_bytes += st.st_size;
    }
    if (stat(mmd0.c_str(), &st) == 0)
    {
        cost.metadata_bytes += st.st_size;
    }
    if (stat(mdidx.c_str(), &st) == 0)
    {
        cost.metadata_bytes += st.st_size;
    }

    // Count data subfiles
    DIR *dir = opendir(path.c_str());
    if (dir)
    {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            if (strncmp(entry->d_name, "data.", 5) == 0)
            {
                cost.subfile_count++;
            }
        }
        closedir(dir);
    }

    return cost;
}

// Evict idle subpools in LRU order. Called reactively when an open fails
// with EMFILE. Must be called with pool_mutex held.
void ADIOSFilePool::EvictLRU()
{
    struct Candidate
    {
        decltype(map)::const_iterator it;
        std::chrono::steady_clock::time_point last_used;
        size_t fd_cost;
    };
    std::vector<Candidate> candidates;

    for (auto it = map.cbegin(); it != map.cend(); ++it)
    {
        auto *subpool = it->second.get();
        std::lock_guard<std::mutex> sub_guard(subpool->subpool_mutex);
        if (subpool->in_use_count == 0)
        {
            candidates.push_back({it, subpool->last_used, subpool->EstimateFDCost()});
        }
    }

    // Sort LRU first (oldest last_used first)
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate &a, const Candidate &b) { return a.last_used < b.last_used; });

    // Evict up to 10% of idle subpools to make room
    size_t to_evict = std::max(candidates.size() / 10, size_t(1));
    size_t evicted = 0;

    for (auto &c : candidates)
    {
        if (evicted >= to_evict)
        {
            break;
        }
        auto *subpool = c.it->second.get();
        std::lock_guard<std::mutex> sub_guard(subpool->subpool_mutex);
        if (subpool->in_use_count != 0)
        {
            continue;
        }
        if (!subpool->m_list.empty())
        {
            std::cout << "Evicting (FD pressure) subpool for \"" << subpool->m_list[0]->m_FileName
                      << "\" (FDs: " << c.fd_cost << ")" << std::endl;
        }
        m_Evictions++;
        m_EvictionsFD++;
        m_TotalMetadataBytes -= subpool->metadata_bytes;
        m_TotalSubfileCount -= subpool->EstimateFDCost();
        map.erase(c.it);
        evicted++;
    }
}

ADIOSFilePool::PoolEntry ADIOSFilePool::GetFree(std::string Filename, bool RowMajorArrays,
                                                const std::string &EngineParams)
{
    m_Requests++;
    std::shared_ptr<SubPool> subpool;
    {
        std::lock_guard<std::mutex> guard(pool_mutex);
        std::string index =
            Filename + "/" + std::to_string((int)RowMajorArrays) + "|" + EngineParams;
        auto res = map.find(index);

        if (res != map.end())
        {
            m_CacheHits++;
            subpool = res->second;
        }
        else
        {
            m_CacheMisses++;
            // Probe cost before creating the new SubPool
            BP5CostEstimate cost = ProbeBP5Directory(Filename, EngineParams);

            // Evict if adding this file would exceed FD or metadata limits
            EvictUnderPressure(cost.is_tar ? 1 : cost.subfile_count, cost.metadata_bytes);

            subpool = std::make_shared<SubPool>();
            subpool->metadata_bytes = cost.metadata_bytes;
            subpool->subfile_count = cost.subfile_count;
            subpool->is_tar = cost.is_tar;

            m_TotalMetadataBytes += cost.metadata_bytes;
            m_TotalSubfileCount += subpool->EstimateFDCost();

            std::cout << "New subpool for \"" << Filename << "\" (metadata: " << cost.metadata_bytes
                      << " bytes, subfiles: " << cost.subfile_count
                      << ", tar: " << (cost.is_tar ? "yes" : "no") << ")" << std::endl;

            map.insert(std::make_pair(index, subpool));
        }
    }
    // pool_mutex is released. The shared_ptr keeps subpool alive even if
    // FlushUnused erases it from the map. The potentially slow file open
    // only blocks other requests for the SAME file, not the whole pool.
    try
    {
        AnonADIOSFile *file = subpool->GetFree(Filename, RowMajorArrays, EngineParams);
        return PoolEntry{file, subpool};
    }
    catch (...)
    {
        // Check if this is an FD exhaustion failure by probing /dev/null
        FILE *probe = std::fopen("/dev/null", "r");
        if (!probe && errno == EMFILE)
        {
            // FD limit hit — evict idle subpools and retry
            std::cout << "FD limit hit, evicting idle subpools..." << std::endl;
            {
                std::lock_guard<std::mutex> guard(pool_mutex);
                EvictLRU();
            }
            AnonADIOSFile *file = subpool->GetFree(Filename, RowMajorArrays, EngineParams);
            return PoolEntry{file, subpool};
        }
        if (probe)
        {
            std::fclose(probe);
        }
        throw; // Not an FD issue — propagate original error
    }
}

AnonADIOSFile *ADIOSFilePool::SubPool::GetFree(std::string Filename, bool RowMajorArrays,
                                               const std::string &EngineParams)
{
    std::lock_guard<std::mutex> guard(subpool_mutex);
    for (size_t i = 0; i < m_busy.size(); i++)
    {
        if (!m_busy[i])
        {
            m_busy[i] = true;
            in_use_count++;
            return m_list[i].get();
        }
    }
    // no free files — open new one (only blocks requests for this same file)
    auto t0 = std::chrono::steady_clock::now();
    m_list.push_back(std::make_unique<AnonADIOSFile>(Filename, RowMajorArrays, EngineParams));
    auto t1 = std::chrono::steady_clock::now();
    open_micros += std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    m_busy.push_back(true);
    in_use_count++;
    return m_list[m_list.size() - 1].get();
}

void ADIOSFilePool::SubPool::Return(adios2::AnonADIOSFile *to_free)
{
    std::lock_guard<std::mutex> guard(subpool_mutex);
    last_used = std::chrono::steady_clock::now();
    for (size_t i = 0; i < m_busy.size(); i++)
    {
        if (to_free == m_list[i].get())
        {
            m_busy[i] = false;
            in_use_count--;
            return;
        }
    }
    std::cerr << "Return FAILED " << std::endl;
}

// Evict idle subpools when FD usage exceeds 90% of m_FDLimit or metadata
// usage exceeds m_MetadataBytesLimit.  Evicts heaviest entries first.
// Called with pool_mutex held.
void ADIOSFilePool::EvictUnderPressure(size_t incoming_fd_cost, size_t incoming_metadata_bytes)
{
    bool fd_over = (m_FDLimit > 0) && (m_TotalSubfileCount + incoming_fd_cost > m_FDLimit * 9 / 10);
    bool md_over = (m_TotalMetadataBytes + incoming_metadata_bytes > m_MetadataBytesLimit);

    if (!fd_over && !md_over)
    {
        return;
    }

    struct Candidate
    {
        decltype(map)::const_iterator it;
        size_t total_cost;
    };
    std::vector<Candidate> candidates;

    for (auto it = map.cbegin(); it != map.cend(); ++it)
    {
        auto *subpool = it->second.get();
        std::lock_guard<std::mutex> sub_guard(subpool->subpool_mutex);
        if (subpool->in_use_count == 0)
        {
            candidates.push_back({it, subpool->EstimateTotalCost()});
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate &a, const Candidate &b) { return a.total_cost > b.total_cost; });

    for (auto &c : candidates)
    {
        fd_over = (m_FDLimit > 0) && (m_TotalSubfileCount + incoming_fd_cost > m_FDLimit * 9 / 10);
        md_over = (m_TotalMetadataBytes + incoming_metadata_bytes > m_MetadataBytesLimit);
        if (!fd_over && !md_over)
        {
            break;
        }
        auto *subpool = c.it->second.get();
        std::lock_guard<std::mutex> sub_guard(subpool->subpool_mutex);
        if (subpool->in_use_count != 0)
        {
            continue;
        }
        if (subpool->m_list.size() > 0)
        {
            std::cout << "Evicting subpool for \"" << subpool->m_list[0]->m_FileName
                      << "\" (metadata: " << subpool->metadata_bytes
                      << " bytes, subfiles: " << subpool->subfile_count
                      << ", reason: " << (fd_over ? "FD" : "metadata") << ")" << std::endl;
        }
        m_Evictions++;
        if (fd_over)
        {
            m_EvictionsFD++;
        }
        if (md_over)
        {
            m_EvictionsMD++;
        }
        m_TotalMetadataBytes -= subpool->metadata_bytes;
        m_TotalSubfileCount -= subpool->EstimateFDCost();
        map.erase(c.it);
    }
}

static std::string HumanBytes(uint64_t bytes)
{
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    double val = static_cast<double>(bytes);
    while (val >= 1024.0 && u < 4)
    {
        val /= 1024.0;
        u++;
    }
    char buf[32];
    if (u == 0)
    {
        snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)bytes);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%.1f %s", val, units[u]);
    }
    return buf;
}

static std::string HumanMicros(uint64_t us)
{
    char buf[32];
    if (us < 1000)
    {
        snprintf(buf, sizeof(buf), "%lluus", (unsigned long long)us);
    }
    else if (us < 1000000)
    {
        snprintf(buf, sizeof(buf), "%.1fms", us / 1000.0);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%.2fs", us / 1000000.0);
    }
    return buf;
}

void ADIOSFilePool::LogStats()
{
    uint64_t reqs = m_Requests.load();
    if (reqs == m_LastStatsReport)
    {
        return;
    }
    m_LastStatsReport = reqs;

    size_t pool_size;
    size_t total_opens = 0;
    uint64_t total_open_us = 0;
    uint64_t total_bytes_served = 0;
    uint64_t total_ops = 0;
    {
        std::lock_guard<std::mutex> guard(pool_mutex);
        pool_size = map.size();
        for (auto &kv : map)
        {
            auto *sp = kv.second.get();
            std::lock_guard<std::mutex> sub_guard(sp->subpool_mutex);
            total_opens += sp->m_list.size();
            total_open_us += sp->open_micros;
            for (auto &f : sp->m_list)
            {
                total_bytes_served += f->m_BytesSent;
                total_ops += f->m_OperationCount;
            }
        }
    }

    std::cout << "Pool stats: requests=" << reqs << " hits=" << m_CacheHits.load()
              << " misses=" << m_CacheMisses.load() << " evictions=" << m_Evictions.load()
              << " (FD=" << m_EvictionsFD.load() << " MD=" << m_EvictionsMD.load()
              << ") cached_files=" << pool_size << " open_handles=" << total_opens
              << " shared_tar_fds=" << adios2::SharedTarFDCache::getInstance().Size()
              << " metadata=" << HumanBytes(m_TotalMetadataBytes) << "/"
              << HumanBytes(m_MetadataBytesLimit) << " est_fds=" << m_TotalSubfileCount << "/"
              << m_FDLimit << " served=" << HumanBytes(total_bytes_served) << " ops=" << total_ops
              << " open_latency=" << HumanMicros(total_open_us)
              << " avg_open=" << HumanMicros(total_opens > 0 ? total_open_us / total_opens : 0)
              << std::endl;
}

void ADIOSFilePool::DoTimeoutTasks()
{
    FlushUnused();
    adios2::SharedTarFDCache::getInstance().CloseIdle();
    LogStats();
}

void ADIOSFilePool::PeriodicTask(std::chrono::milliseconds interval)
{
    while (!(m_ShutdownFlag))
    {
        DoTimeoutTasks();
        std::this_thread::sleep_for(interval);
    }
    std::cout << "Clean shutdown of ADIOSFilePool periodic thread" << std::endl;
}

ADIOSFilePool::ADIOSFilePool()
{
    // FD limit: env override > rlimit, with 90% headroom for XRootD overhead
    const char *fd_env = std::getenv("ADIOS_POOL_FD_LIMIT");
    if (fd_env)
    {
        m_FDLimit = std::strtoull(fd_env, nullptr, 10);
    }
    else
    {
        struct rlimit rl;
        if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
        {
            if (rl.rlim_cur < rl.rlim_max)
            {
                rl.rlim_cur = rl.rlim_max;
                setrlimit(RLIMIT_NOFILE, &rl);
                getrlimit(RLIMIT_NOFILE, &rl);
            }
            m_FDLimit = static_cast<size_t>(rl.rlim_cur) * 9 / 10;
        }
    }

    // Metadata limit: env override > default (2 GB)
    const char *md_env = std::getenv("ADIOS_POOL_METADATA_LIMIT");
    if (md_env)
    {
        m_MetadataBytesLimit = std::strtoull(md_env, nullptr, 10);
    }

    adios2::SharedTarFDCache::Enable();
    std::cout << "ADIOSFilePool created (FD limit: " << m_FDLimit
              << ", metadata limit: " << m_MetadataBytesLimit << ", SharedTarFDCache enabled).\n";
    periodicThread = periodicWorker();
}

ADIOSFilePool &ADIOSFilePool::getInstance()
{
    static ADIOSFilePool instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
}

void ADIOSFilePool::FlushUnused()
{
    std::lock_guard<std::mutex> guard(pool_mutex);
    EvictUnderPressure(0, 0);
}

ADIOSFilePool::~ADIOSFilePool()
{
    m_ShutdownFlag = true;
    if (periodicThread.joinable())
    {
        periodicThread.join();
    }
}
} // end of namespace adios2
