/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Transport.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: wfg
 */

#include "Transport.h"
#include "adios2/core/CoreTypes.h"

#include "adios2/helper/adiosFunctions.h" //CreateDirectory

namespace adios2
{

Transport::Transport(const std::string type, const std::string library,
                     helper::Comm const &comm)
: m_Type(type), m_Library(library), m_Comm(comm)
{
}

void Transport::IWrite(const char *buffer, size_t size, Status &status,
                       size_t start)
{
    throw std::invalid_argument("ERROR: this class doesn't implement IWrite\n");
}

void Transport::WriteV(const core::iovec *iov, const int iovcnt, size_t start)
{
    if (iovcnt > 0)
    {
        Write(static_cast<const char *>(iov[0].iov_base), iov[0].iov_len,
              start);
        for (int c = 1; c < iovcnt; ++c)
        {
            Write(static_cast<const char *>(iov[c].iov_base), iov[c].iov_len);
        }
    }
    else if (start != MaxSizeT)
    {
        Seek(start);
    }
}

void Transport::WriteV(const core::iovec *iov, const int iovcnt,
                       const size_t totalsize, const double deadline_sec,
                       bool *flagRush, size_t start)
{
    core::TimePoint starttime = core::Now();
    /*std::cout << "WriteV totalsize = " << totalsize
              << " deadline_sec = " << deadline_sec << std::endl;*/

    // Set deadline 95% of allotted time but also discount 0.01s real time
    // for the extra hassle
    double internalDeadlineSec = deadline_sec * 0.95 - 0.01;
    if (internalDeadlineSec < 0.0)
    {
        internalDeadlineSec = 0.0;
    }

    if (iovcnt == 0 || internalDeadlineSec == 0.0 || *flagRush)
    {
        WriteV(iov, iovcnt, start);
        core::Seconds totalWriteTime = core::Now() - starttime;
        std::cout << "WriteV sync spent total time = " << totalWriteTime.count()
                  << std::endl;
        return;
    }

    core::Seconds deadlineSeconds = core::Seconds(internalDeadlineSec);

    size_t wrote = 0;
    int block = 0;
    size_t temp_offset = 0;
    size_t max_size = 1024 * 1024;
    bool firstwrite = true;
    core::Seconds writeTotalTime(0.0);
    core::Seconds sleepTotalTime(0.0);
    while (block < iovcnt)
    {
        if (!firstwrite && max_size < MaxSizeT)
        {
            core::Seconds timesofar = core::Now() - starttime;
            /*std::cout << "  Wrote = " << wrote
                      << " time so far = " << timesofar.count() << std::endl;*/
            if (timesofar > deadlineSeconds || *flagRush)
            {
                // Passed the deadline, write the rest without any waiting
                std::cout << "  Passed deadline, time so far = "
                          << timesofar.count() << std::endl;
                max_size = MaxSizeT;
            }
            else
            {
                // estimate required future write time and
                // sleep for a portion of the available free time
                double bw = wrote / writeTotalTime.count(); // bytes per second
                double needtime = (totalsize - wrote) / bw;
                double availabletime = (deadlineSeconds - timesofar).count();
                if (availabletime < needtime)
                {
                    // not enough time with this rate, increase max_size
                    max_size *= 2;
                    double t = needtime / 4.0;
                    while (availabletime < t)
                    {
                        // Note that doubling the block size DOES NOT guarantee
                        // higher bandwidth let alone double bw. This is just
                        // a desperate attempt to speed up the output
                        max_size *= 2;
                        t /= 2.0;
                    }
                    std::cout
                        << "    We are behind, time left =  " << availabletime
                        << " need time = " << needtime
                        << " increase block size = " << max_size << std::endl;
                }
                else
                {
                    // sleep for 1/Kth of estimated free time, where still K
                    // block writes are to be done
                    double futureTotalSleepTime = (availabletime - needtime);
                    size_t futureWritesNum = (totalsize - wrote) / max_size;
                    size_t rem = (totalsize - wrote) % max_size;
                    if (rem > 0)
                    {
                        ++futureWritesNum;
                    }
                    double sleepTime = futureTotalSleepTime / futureWritesNum;
                    /*std::cout
                        << "    We have time to throttle, time left =  "
                        << availabletime << " need time = " << needtime
                        << " number of future writes = " << futureWritesNum
                        << " sleep now = " << sleepTime << std::endl;*/
                    std::this_thread::sleep_for(core::Seconds(sleepTime));
                }
            }
        }
        core::TimePoint writeStart = core::Now();
        /* Write up to max_size bytes from the current block */
        size_t n = iov[block].iov_len - temp_offset;
        if (n > max_size)
        {
            n = max_size;
        }
        if (firstwrite)
        {
            /*std::cout << "  Write start = " << start << " size = " << n
                      << " block = " << block << " len = " << iov[block].iov_len
                      << " tempt_offset = " << temp_offset << std::endl;*/
            Write(static_cast<const char *>(iov[block].iov_base) + temp_offset,
                  n, start);
            firstwrite = false;
        }
        else
        {
            /*std::cout << "  Write"
                      << " size = " << n << " block = " << block
                      << " temp_offset = " << temp_offset << std::endl;*/
            Write(static_cast<const char *>(iov[block].iov_base) + temp_offset,
                  n);
            Flush();
        }

        /* Have we processed the entire block or staying with it? */
        if (n + temp_offset < iov[block].iov_len)
        {
            temp_offset += n;
        }
        else
        {
            temp_offset = 0;
            ++block;
        }
        wrote += n;
        writeTotalTime += core::Now() - writeStart;
    }

    core::Seconds totalWriteTime = core::Now() - starttime;
    std::cout << "WriteV spent total time = " << totalWriteTime.count()
              << " deadline was = " << deadlineSeconds.count() << std::endl;
}

void Transport::IRead(char *buffer, size_t size, Status &status, size_t start)
{
    throw std::invalid_argument("ERROR: this class doesn't implement IRead\n");
}

void Transport::InitProfiler(const Mode openMode, const TimeUnit timeUnit)
{
    m_Profiler.m_IsActive = true;

    m_Profiler.m_Timers.emplace(std::make_pair(
        "open", profiling::Timer("open", TimeUnit::Microseconds)));

    if (openMode == Mode::Write)
    {
        m_Profiler.m_Timers.emplace("write",
                                    profiling::Timer("write", timeUnit));

        m_Profiler.m_Bytes.emplace("write", 0);
    }
    else if (openMode == Mode::Append)
    {
        /*
        m_Profiler.Timers.emplace(
            "append", profiling::Timer("append", timeUnit));
        m_Profiler.Bytes.emplace("append", 0);
        */
        m_Profiler.m_Timers.emplace("write",
                                    profiling::Timer("write", timeUnit));

        m_Profiler.m_Bytes.emplace("write", 0);

        m_Profiler.m_Timers.emplace("read", profiling::Timer("read", timeUnit));

        m_Profiler.m_Bytes.emplace("read", 0);
    }
    else if (openMode == Mode::Read)
    {
        m_Profiler.m_Timers.emplace("read", profiling::Timer("read", timeUnit));
        m_Profiler.m_Bytes.emplace("read", 0);
    }

    m_Profiler.m_Timers.emplace(
        "close", profiling::Timer("close", TimeUnit::Microseconds));
}

void Transport::OpenChain(const std::string &name, const Mode openMode,
                          const helper::Comm &chainComm, const bool async)
{
    std::invalid_argument("ERROR: " + m_Name + " transport type " + m_Type +
                          " using library " + m_Library +
                          " doesn't implement the OpenChain function\n");
}

void Transport::SetParameters(const Params &parameters) {}

void Transport::SetBuffer(char * /*buffer*/, size_t /*size*/)
{
    std::invalid_argument("ERROR: " + m_Name + " transport type " + m_Type +
                          " using library " + m_Library +
                          " doesn't implement the SetBuffer function\n");
}

void Transport::Flush()
{
    std::invalid_argument("ERROR: " + m_Name + " transport type " + m_Type +
                          " using library " + m_Library +
                          " doesn't implement the Flush function\n");
}

size_t Transport::GetSize() { return 0; }

void Transport::ProfilerStart(const std::string process) noexcept
{
    if (m_Profiler.m_IsActive)
    {
        m_Profiler.m_Timers.at(process).Resume();
    }
}

void Transport::ProfilerStop(const std::string process) noexcept
{
    if (m_Profiler.m_IsActive)
    {
        m_Profiler.m_Timers.at(process).Pause();
    }
}

void Transport::CheckName() const
{
    if (m_Name.empty())
    {
        throw std::invalid_argument("ERROR: name can't be empty for " +
                                    m_Library + " transport \n");
    }
}

} // end namespace adios2
