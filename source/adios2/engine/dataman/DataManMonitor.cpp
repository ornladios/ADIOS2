/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManMonitor.cpp
 *
 *  Created on: Jun 2, 2020
 *      Author: Jason Wang
 */

#include "DataManMonitor.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace adios2
{
namespace core
{
namespace engine
{

void DataManMonitor::SetAverageSteps(const size_t step)
{
    m_AverageSteps = step;
}

void DataManMonitor::SetCombiningSteps(const size_t step)
{
    m_CombiningSteps = step;
}

void DataManMonitor::SetClockError(const uint64_t roundLatency,
                                   const uint64_t remoteTimeBase)
{
    uint64_t localTimeBase =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    m_ClockError = localTimeBase - remoteTimeBase -
                   static_cast<double>(roundLatency) / 2.0;
    m_RoundLatency = roundLatency;
}

void DataManMonitor::BeginStep(const size_t step)
{
    if (step == 0)
    {
        m_InitialTimer = std::chrono::system_clock::now();
    }
    if (m_StepTimers.empty())
    {
        m_StepTimers.push(std::chrono::system_clock::now());
    }

    m_StepBytes = 0;

    if (m_TotalBytes.empty())
    {
        m_TotalBytes.push(0);
    }
    else
    {
        m_TotalBytes.push(m_TotalBytes.back());
    }

    ++m_CurrentStep;
}

void DataManMonitor::AddLatencyMilliseconds(const uint64_t remoteStamp)
{
    uint64_t localStamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    uint64_t latency = localStamp - remoteStamp - m_ClockError;
    m_LatencyMilliseconds.push_back(latency);
    m_AccumulatedLatency += latency;
}

void DataManMonitor::EndStep(const size_t step)
{
    m_StepTimers.push(std::chrono::system_clock::now());

    while (m_StepTimers.size() > m_AverageSteps)
    {
        m_StepTimers.pop();
    }
    while (m_TotalBytes.size() > m_AverageSteps)
    {
        m_TotalBytes.pop();
    }
    while (m_LatencyMilliseconds.size() > m_AverageSteps)
    {
        m_LatencyMilliseconds.pop_front();
    }

    m_TotalTime = std::chrono::duration_cast<std::chrono::microseconds>(
                      (m_StepTimers.back() - m_InitialTimer))
                      .count();
    m_AverageTime = std::chrono::duration_cast<std::chrono::microseconds>(
                        (m_StepTimers.back() - m_StepTimers.front()))
                        .count();
    m_TotalRate = static_cast<double>(m_TotalBytes.back()) /
                  static_cast<double>(m_TotalTime);
    m_AverageRate =
        static_cast<double>(m_TotalBytes.back() - m_TotalBytes.front()) /
        static_cast<double>(m_AverageTime);
    if (step > 0)
    {
        m_DropRate = static_cast<double>((step - m_CurrentStep)) / step;
    }
    m_StepsPerSecond = step / m_TotalTime * 1000000;

    double averageLatency = 0;
    if (m_LatencyMilliseconds.size() > 0)
    {
        for (const auto &l : m_LatencyMilliseconds)
        {
            averageLatency = averageLatency + l;
        }
        averageLatency = averageLatency / m_LatencyMilliseconds.size();
    }

    if (m_Verbose)
    {
        std::lock_guard<std::mutex> l(m_PrintMutex);
        std::cout << "Step " << step << ", Total MBs "
                  << static_cast<double>(m_TotalBytes.back()) / 1000000.0
                  << ", Step MBs "
                  << static_cast<double>(m_StepBytes) / 1000000.0
                  << ", Total seconds "
                  << static_cast<double>(m_TotalTime) / 1000000.0 << ", "
                  << m_StepTimers.size() << " step seconds "
                  << static_cast<double>(m_AverageTime) / 1000000.0
                  << ", Total MB/s " << m_TotalRate << ", "
                  << m_StepTimers.size() << " step average MB/s "
                  << m_AverageRate << ", Drop rate " << m_DropRate * 100 << "%"
                  << ", Steps per second " << m_StepsPerSecond << ", "
                  << m_StepTimers.size()
                  << " step average latency milliseconds " << averageLatency
                  << ", Average latency milliseconds "
                  << m_AccumulatedLatency /
                         static_cast<double>(m_CurrentStep + 1)
                  << std::endl;
    }
}

void DataManMonitor::AddBytes(const size_t bytes)
{
    m_TotalBytes.back() += bytes;
    m_StepBytes += bytes;
}

void DataManMonitor::SetRequiredAccuracy(const std::string &accuracyRequired)
{
    m_RequiredAccuracy = accuracyRequired;
}

void DataManMonitor::AddCompression(const std::string &method,
                                    const std::string &accuracyUsed)
{
    m_CompressionMethod = method;
    m_CompressionAccuracy = accuracyUsed;
}

void DataManMonitor::AddTransport(const std::string &method)
{
    m_TransportMethod = method;
}

void DataManMonitor::SetReaderThreading() { m_ReaderThreading = true; }

void DataManMonitor::SetWriterThreading() { m_WriterThreading = true; }

void DataManMonitor::OutputJson(const std::string &filename)
{
    nlohmann::json output;

    output["Performance"]["Bandwidth"] = m_TotalRate;
    output["Performance"]["Latency"] =
        m_AccumulatedLatency / static_cast<double>(m_CurrentStep + 1);
    output["Performance"]["DropRate"] = m_DropRate;
    output["Performance"]["StepDataSize"] = m_StepBytes;
    output["Performance"]["AllowedError"] = m_RequiredAccuracy;

    output["Decisions"]["CombiningSteps"] = m_CombiningSteps;
    output["Decisions"]["ReaderThreading"] = m_ReaderThreading;
    output["Decisions"]["WriterThreading"] = m_WriterThreading;
    output["Decisions"]["Transport"] = m_TransportMethod;
    if (!m_CompressionMethod.empty())
    {
        output["Decisions"]["CompressionMethod"] = m_CompressionMethod;
        output["Decisions"]["CompressionAccuracy"] = m_CompressionAccuracy;
    }

    output["Info"]["RoundLatency"] = m_RoundLatency;
    output["Info"]["ClockError"] = m_ClockError;

    std::ofstream file;
    file.open((filename + ".json").c_str(),
              std::fstream::out | std::fstream::app);
    file << output.dump() << "\0" << std::endl;
    file.close();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
