/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOSFILEPOOL_H_
#define ADIOSFILEPOOL_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <sys/resource.h>
#include <thread>
#include <unordered_map>
#include <vector>
/// \endcond

#include "adios2.h"
#include "adios2/toolkit/filepool/SharedTarFDCache.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

using namespace adios2::core;
using namespace adios2;

namespace adios2
{

struct BP5CostEstimate
{
    size_t metadata_bytes = 0; // sum of md.0 + mmd.0 + md.idx sizes
    size_t subfile_count = 0;  // number of data.* files
    bool is_tar = false;       // if true, FD cost is ~1 regardless of subfile_count
};

class AnonADIOSFile
{
public:
    adios2::ADIOS adios;
    adios2::IO m_io;
    adios2::Engine m_engine;
    int64_t m_ID;
    std::string m_IOname;
    std::string m_FileName;
    bool m_RowMajorArrays;
    std::atomic<size_t> m_BytesSent{0};
    std::atomic<size_t> m_OperationCount{0};
    AnonADIOSFile(std::string FileName, bool RowMajorArrays,
                  const std::string &EngineParams = std::string())
    {
        Mode adios_read_mode = adios2::Mode::Read;
        m_FileName = FileName;
        m_IOname = RandomString(8);
        m_RowMajorArrays = RowMajorArrays;
        ArrayOrdering ArrayOrder =
            RowMajorArrays ? ArrayOrdering::RowMajor : ArrayOrdering::ColumnMajor;
        m_io = adios.DeclareIO(m_IOname, ArrayOrder);
        if (!EngineParams.empty())
        {
            // Decode TAB-separated key=value pairs and set as engine parameters
            std::stringstream ss(EngineParams);
            std::string entry;
            while (std::getline(ss, entry, '\t'))
            {
                auto eqPos = entry.find('=');
                if (eqPos != std::string::npos && eqPos > 0)
                {
                    m_io.SetParameter(entry.substr(0, eqPos), entry.substr(eqPos + 1));
                }
            }
        }
        adios_read_mode = adios2::Mode::ReadRandomAccess;
        m_engine = m_io.Open(FileName, adios_read_mode);
        std::memcpy(&m_ID, m_IOname.c_str(), sizeof(m_ID));
    }
    ~AnonADIOSFile()
    {
        m_engine.Close();
        adios.RemoveIO(m_IOname);
    }

private:
    std::string RandomString(const size_t length)
    {
        size_t len = length;
        if (len == 0)
            len = 1;
        if (len > 64)
            len = 64;

        std::string str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzA");

        std::random_device rd;
        std::mt19937 generator(rd());

        std::shuffle(str.begin(), str.end(), generator);

        return str.substr(0, len);
    }
};

// singleton
class ADIOSFilePool
{
public:
    class SubPool
    {
    public:
        SubPool() = default;
        ~SubPool() = default;
        std::mutex subpool_mutex;
        std::chrono::steady_clock::time_point last_used;
        size_t in_use_count = 0;
        std::vector<std::unique_ptr<AnonADIOSFile>> m_list;
        std::vector<bool> m_busy;

        // Resource cost estimates (set once at creation, never change)
        size_t metadata_bytes = 0;
        size_t subfile_count = 0;
        bool is_tar = false;
        uint64_t open_micros = 0; // cumulative file open latency

        AnonADIOSFile *GetFree(std::string Filename, bool RowMajorArrays,
                               const std::string &EngineParams = std::string());
        void Return(AnonADIOSFile *Entry);

        size_t EstimateFDCost() const
        {
            if (is_tar)
            {
                // With SharedTarFDCache, tar FDs are shared across engines
                return adios2::SharedTarFDCache::IsEnabled() ? 0 : 1;
            }
            return subfile_count;
        }
        size_t EstimateTotalCost() const { return metadata_bytes + EstimateFDCost(); }
    };

    // RAII wrapper: automatically returns the file to the pool on scope exit.
    // Move-only; moving transfers ownership (source becomes empty).
    struct PoolEntry
    {
        AnonADIOSFile *file = nullptr;
        std::shared_ptr<SubPool> subpool;

        PoolEntry() = default;
        PoolEntry(AnonADIOSFile *f, std::shared_ptr<SubPool> sp) : file(f), subpool(sp) {}
        ~PoolEntry()
        {
            if (file && subpool)
            {
                subpool->Return(file);
            }
        }
        // Move-only
        PoolEntry(PoolEntry &&o) noexcept : file(o.file), subpool(std::move(o.subpool))
        {
            o.file = nullptr;
        }
        PoolEntry &operator=(PoolEntry &&o) noexcept
        {
            if (this != &o)
            {
                if (file && subpool)
                {
                    subpool->Return(file);
                }
                file = o.file;
                subpool = std::move(o.subpool);
                o.file = nullptr;
            }
            return *this;
        }
        PoolEntry(const PoolEntry &) = delete;
        PoolEntry &operator=(const PoolEntry &) = delete;
    };

    static ADIOSFilePool &getInstance();
    ~ADIOSFilePool();

    PoolEntry GetFree(std::string Filename, bool RowMajorArrays,
                      const std::string &EngineParams = std::string());
    void FlushUnused();

private:
    static ADIOSFilePool *instance;

    // Private constructor
    ADIOSFilePool();

    // Delete copy constructor and assignment
    ADIOSFilePool(const ADIOSFilePool &) = delete;
    ADIOSFilePool &operator=(const ADIOSFilePool &) = delete;

    void PeriodicTask(std::chrono::milliseconds interval);
    void DoTimeoutTasks();
    std::atomic<bool> m_ShutdownFlag{false};
    std::thread periodicWorker()
    {
        return std::thread([=] { PeriodicTask(std::chrono::milliseconds(1000)); });
    }
    std::thread periodicThread;

    // Resource monitoring
    size_t m_FDLimit = 0;
    size_t m_MetadataBytesLimit = 1ULL << 31; // 2 GB default
    size_t m_TotalMetadataBytes = 0;
    size_t m_TotalSubfileCount = 0;
    void EvictUnderPressure(size_t incoming_fd_cost, size_t incoming_metadata_bytes);
    void EvictLRU();

    static BP5CostEstimate ProbeBP5Directory(const std::string &path,
                                             const std::string &EngineParams);
    static BP5CostEstimate ParseTarInfoCost(const std::string &tarinfo);

    // Statistics counters
    std::atomic<uint64_t> m_CacheHits{0};
    std::atomic<uint64_t> m_CacheMisses{0};
    std::atomic<uint64_t> m_Evictions{0};
    std::atomic<uint64_t> m_EvictionsFD{0};
    std::atomic<uint64_t> m_EvictionsMD{0};
    std::atomic<uint64_t> m_Requests{0};
    uint64_t m_LastStatsReport = 0; // m_Requests value at last log
    void LogStats();

    std::mutex pool_mutex;
    std::unordered_map<std::string, std::shared_ptr<SubPool>> map;
};

} // end namespace adios2

#endif /* ADIOSFILEPOOL_H_ */
