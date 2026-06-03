/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_UTILS_XROOTD_ACCESSLOG_H_
#define ADIOS2_UTILS_XROOTD_ACCESSLOG_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/*
 * Per-request access log for the ADIOS XRootD service: one JSON object per line,
 * for later analysis of which variables/steps/regions get read.
 *
 * Off unless ADIOS2_XROOTD_ACCESSLOG names a writable file.
 *
 * Never blocks a request thread: workers format a line and hand it off under a
 * brief lock; a dedicated thread does all file I/O off the lock. The handoff
 * queue is bounded and drops (counted) rather than blocking if a worker outruns
 * the writer.
 */
class AccessLog
{
public:
    static AccessLog &Instance();

    /* Cheap check to gate record construction at the call site. */
    bool Enabled() const { return m_Enabled.load(std::memory_order_relaxed); }

    /* Pointers reference caller-owned data and need only outlive the call;
     * Log() formats synchronously before returning. */
    struct Record
    {
        const char *file = nullptr;
        const std::string *var = nullptr;
        uint64_t step = 0;
        uint64_t stepCount = 1;
        const std::vector<size_t> *start = nullptr; // null/empty => whole-variable
        const std::vector<size_t> *count = nullptr;
        uint64_t blockID = static_cast<uint64_t>(-1); // -1 => no block selection
        double accuracyError = 0.0;
        uint64_t bytes = 0;
        uint32_t batch = 1; // number of variables in the batch this belonged to
    };

    /* Record one request. No-op when disabled; may drop if the queue is full. */
    void Log(const Record &r);

private:
    AccessLog();
    ~AccessLog();
    AccessLog(const AccessLog &) = delete;
    AccessLog &operator=(const AccessLog &) = delete;

    void WriterLoop();
    // The following touch m_File and the rotation state, which only the writer
    // thread ever touches, so they take no lock.
    void WriteHeader();
    void Rotate();
    void PruneSegments();

    static constexpr size_t kMaxQueue = 65536; // drop beyond this many pending lines
    static constexpr size_t kDefaultMaxSize = 128 * 1024 * 1024; // rotate active file at 128 MB
    static constexpr size_t kDefaultKeep = 16; // rotated segments to retain (~2 GB)

    std::atomic<bool> m_Enabled{false};
    std::atomic<bool> m_Running{false};
    size_t m_MaxQueue = kMaxQueue;

    // Rotation config + state; writer-thread-owned after construction.
    std::string m_Path;                 // active log file (stable name)
    size_t m_MaxSize = kDefaultMaxSize; // 0 => never rotate
    size_t m_Keep = kDefaultKeep;
    size_t m_CurrentSize = 0;

    std::ofstream m_File; // writer-thread-owned
    std::thread m_Writer;

    std::mutex m_Mutex; // guards m_Active and m_Dropped
    std::condition_variable m_CV;
    std::vector<std::string> m_Active;
    uint64_t m_Dropped = 0;
};

#endif /* ADIOS2_UTILS_XROOTD_ACCESSLOG_H_ */
