/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "AccessLog.h"

#include <algorithm>
#include <chrono>
#include <cstdio>  // rename, remove
#include <cstdlib> // getenv, strtol
#include <ctime>   // gmtime_r, strftime
#include <sstream>

#include <dirent.h> // opendir/readdir for retention
#include <unistd.h> // access

namespace
{

// Append s as a JSON string literal, escaping the few characters that matter.
void AppendJsonString(std::ostringstream &os, const char *s)
{
    os << '"';
    for (; s && *s; ++s)
    {
        const unsigned char c = static_cast<unsigned char>(*s);
        if (c == '"' || c == '\\')
            os << '\\' << static_cast<char>(c);
        else if (c < 0x20)
            os << ' '; // drop control chars rather than emit invalid JSON
        else
            os << static_cast<char>(c);
    }
    os << '"';
}

void AppendDims(std::ostringstream &os, const std::vector<size_t> &dims)
{
    os << '[';
    for (size_t i = 0; i < dims.size(); ++i)
    {
        if (i)
            os << ',';
        os << dims[i];
    }
    os << ']';
}

// UTC timestamp suffix for rotated segments; sorts chronologically.
std::string UtcStamp()
{
    const std::time_t t = std::time(nullptr);
    std::tm tm;
    gmtime_r(&t, &tm);
    char buf[24];
    std::strftime(buf, sizeof(buf), "%Y%m%dT%H%M%S", &tm);
    return std::string(buf);
}

} // namespace

AccessLog &AccessLog::Instance()
{
    static AccessLog instance;
    return instance;
}

AccessLog::AccessLog()
{
    const char *path = std::getenv("ADIOS2_XROOTD_ACCESSLOG");
    if (!path || !*path)
        return; // disabled

    const char *maxsize = std::getenv("ADIOS2_XROOTD_ACCESSLOG_MAXSIZE");
    if (maxsize)
    {
        const long long n = std::strtoll(maxsize, nullptr, 10);
        if (n >= 0)
            m_MaxSize = static_cast<size_t>(n); // 0 => never rotate
    }

    const char *keep = std::getenv("ADIOS2_XROOTD_ACCESSLOG_KEEP");
    if (keep)
    {
        const long n = std::strtol(keep, nullptr, 10);
        if (n >= 0)
            m_Keep = static_cast<size_t>(n);
    }

    m_File.open(path, std::ios::out | std::ios::app);
    if (!m_File.is_open())
        return; // can't open => stay disabled

    m_Path = path;
    m_File.seekp(0, std::ios::end);
    const std::streampos pos = m_File.tellp();
    m_CurrentSize = (pos > 0) ? static_cast<size_t>(pos) : 0;
    if (m_CurrentSize == 0)
        WriteHeader(); // fresh file gets a schema line; an appended one keeps its own

    m_Running.store(true);
    m_Writer = std::thread(&AccessLog::WriterLoop, this);
    m_Enabled.store(true, std::memory_order_relaxed);
}

AccessLog::~AccessLog()
{
    m_Enabled.store(false, std::memory_order_relaxed);
    if (!m_Running.exchange(false))
        return;
    m_CV.notify_all();
    if (m_Writer.joinable())
        m_Writer.join();
}

void AccessLog::Log(const Record &r)
{
    if (!m_Enabled.load(std::memory_order_relaxed))
        return;

    const double ts =
        std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();

    // Formatted synchronously on the caller's thread: the Record's pointers are
    // dereferenced here, while the caller is still blocked, and only the
    // finished string is handed to the writer thread. Do not defer this.
    std::ostringstream os;
    os << "{\"ts\":" << ts;
    if (r.file)
    {
        os << ",\"file\":";
        AppendJsonString(os, r.file);
    }
    if (r.var)
    {
        os << ",\"var\":";
        AppendJsonString(os, r.var->c_str());
    }
    os << ",\"step\":" << r.step << ",\"nsteps\":" << r.stepCount;
    if (r.start && !r.start->empty())
    {
        os << ",\"start\":";
        AppendDims(os, *r.start);
    }
    if (r.count && !r.count->empty())
    {
        os << ",\"count\":";
        AppendDims(os, *r.count);
    }
    if (r.blockID != static_cast<uint64_t>(-1))
        os << ",\"block\":" << r.blockID;
    if (r.accuracyError != 0.0)
        os << ",\"acc\":" << r.accuracyError;
    os << ",\"bytes\":" << r.bytes << ",\"batch\":" << r.batch << "}";

    std::string line = os.str();
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_Active.size() < m_MaxQueue)
            m_Active.push_back(std::move(line));
        else
            ++m_Dropped;
    }
    m_CV.notify_one();
}

void AccessLog::WriteHeader()
{
    static const std::string h = "{\"_schema\":1}\n";
    m_File << h;
    m_File.flush();
    m_CurrentSize += h.size();
}

void AccessLog::Rotate()
{
    m_File.flush();
    m_File.close();

    std::string target = m_Path + "." + UtcStamp();
    if (access(target.c_str(), F_OK) == 0)
    {
        // Same-second collision (only plausible under extreme load): disambiguate.
        int seq = 1;
        std::string candidate;
        do
        {
            candidate = target + "-" + std::to_string(seq++);
        } while (access(candidate.c_str(), F_OK) == 0);
        target = candidate;
    }
    std::rename(m_Path.c_str(), target.c_str());

    m_File.open(m_Path, std::ios::out | std::ios::trunc);
    m_CurrentSize = 0;
    if (m_File.is_open())
        WriteHeader();
    PruneSegments();
}

void AccessLog::PruneSegments()
{
    std::string dir;
    std::string base;
    const size_t slash = m_Path.find_last_of('/');
    if (slash == std::string::npos)
    {
        dir = ".";
        base = m_Path;
    }
    else
    {
        dir = m_Path.substr(0, slash);
        base = m_Path.substr(slash + 1);
    }
    const std::string prefix = base + "."; // matches segments, not the active file

    DIR *d = opendir(dir.c_str());
    if (!d)
        return;
    std::vector<std::string> segments;
    for (struct dirent *e = readdir(d); e != nullptr; e = readdir(d))
    {
        const std::string name = e->d_name;
        if (name.size() > prefix.size() && name.compare(0, prefix.size(), prefix) == 0)
            segments.push_back(name);
    }
    closedir(d);

    if (segments.size() <= m_Keep)
        return;
    std::sort(segments.begin(), segments.end()); // chronological by timestamp suffix
    const size_t toDelete = segments.size() - m_Keep;
    for (size_t i = 0; i < toDelete; ++i)
        std::remove((dir + "/" + segments[i]).c_str());
}

void AccessLog::WriterLoop()
{
    std::vector<std::string> batch;
    bool running = true;
    while (running)
    {
        uint64_t dropped = 0;
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_CV.wait_for(lock, std::chrono::milliseconds(250),
                          [this] { return !m_Active.empty() || !m_Running.load(); });
            batch.swap(m_Active);
            dropped = m_Dropped;
            m_Dropped = 0;
            running = m_Running.load();
        }
        bool wrote = false;
        for (const auto &line : batch)
        {
            m_File << line << '\n';
            m_CurrentSize += line.size() + 1;
            wrote = true;
        }
        if (dropped)
        {
            std::ostringstream os;
            os << "{\"dropped\":" << dropped << "}\n";
            const std::string d = os.str();
            m_File << d;
            m_CurrentSize += d.size();
            wrote = true;
        }
        if (wrote)
            m_File.flush();
        batch.clear();

        if (m_MaxSize > 0 && m_CurrentSize > m_MaxSize)
            Rotate();
    }

    // Shutting down: drain anything enqueued after the last swap. Request
    // threads are quiescent by now, so one final pass suffices.
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto &line : m_Active)
        m_File << line << '\n';
    if (m_Dropped)
        m_File << "{\"dropped\":" << m_Dropped << "}\n";
    m_File.flush();
}
