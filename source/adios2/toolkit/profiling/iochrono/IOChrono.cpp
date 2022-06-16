/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IOChrono.cpp
 *
 *  Created on: Sep 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "IOChrono.h"
#include "adios2/helper/adiosMemory.h"

namespace adios2
{
namespace profiling
{

void IOChrono::Start(const std::string process) noexcept
{
    if (m_IsActive)
    {
        m_Timers.at(process).Resume();
    }
}

void IOChrono::Stop(const std::string process)
{
    if (m_IsActive)
    {
        m_Timers.at(process).Pause();
    }
}

//
// class JSON Profiler
//
JSONProfiler::JSONProfiler(helper::Comm const &comm) : m_Comm(comm)
{
    m_Profiler.m_IsActive = true; // default is true

    AddTimerWatch("buffering");
    AddTimerWatch("endstep");
    AddTimerWatch("PP");
    AddTimerWatch("meta_gather1", false);
    AddTimerWatch("meta_gather2", false);
    AddTimerWatch("meta_lvl1");
    AddTimerWatch("meta_lvl2");
    AddTimerWatch("close_ts");
    AddTimerWatch("AWD");
    AddTimerWatch("WaitOnAsync");

    m_Profiler.m_Bytes.emplace("buffering", 0);

    m_RankMPI = m_Comm.Rank();
}

void JSONProfiler::AddTimerWatch(const std::string &name, const bool trace)
{
    const TimeUnit timerUnit = DefaultTimeUnitEnum;
    m_Profiler.m_Timers.emplace(name, profiling::Timer(name, timerUnit, trace));
}

std::string JSONProfiler::GetRankProfilingJSON(
    const std::vector<std::string> &transportsTypes,
    const std::vector<profiling::IOChrono *> &transportsProfilers) noexcept
{
    auto lf_WriterTimer = [](std::string &rankLog,
                             const profiling::Timer &timer) {
        // rankLog += "\"" + timer.m_Process + "_" + timer.GetShortUnits() +
        //           "\": " + std::to_string(timer.m_ProcessTime) + ", ";
        timer.AddToJsonStr(rankLog);
    };

    // prepare string dictionary per rank
    std::string rankLog("{ \"rank\":" + std::to_string(m_RankMPI));

    auto &profiler = m_Profiler;

    std::string timeDate(profiler.m_Timers.at("buffering").m_LocalTimeDate);
    timeDate.pop_back();
    // avoid whitespace
    std::replace(timeDate.begin(), timeDate.end(), ' ', '_');

    rankLog += ", \"start\":\"" + timeDate + "\"";
    // rankLog += ", \"threads\":" + std::to_string(m_Parameters.Threads);
    rankLog +=
        ", \"bytes\":" + std::to_string(profiler.m_Bytes.at("buffering"));

    for (const auto &timerPair : profiler.m_Timers)
    {
        const profiling::Timer &timer = timerPair.second;
        // rankLog += "\"" + timer.m_Process + "_" + timer.GetShortUnits() +
        //          "\": " + std::to_string(timer.m_ProcessTime) + ", ";
        timer.AddToJsonStr(rankLog);
    }

    const size_t transportsSize = transportsTypes.size();

    for (unsigned int t = 0; t < transportsSize; ++t)
    {
        rankLog += ", \"transport_" + std::to_string(t) + "\":{";
        rankLog += "\"type\":\"" + transportsTypes[t] + "\"";

        for (const auto &transportTimerPair : transportsProfilers[t]->m_Timers)
        {
            lf_WriterTimer(rankLog, transportTimerPair.second);
        }
        rankLog += "}";
    }
    rankLog += " }"; // end rank entry

    return rankLog;
}

std::vector<char>
JSONProfiler::AggregateProfilingJSON(const std::string &rankLog) const
{
    // Gather sizes
    const size_t rankLogSize = rankLog.size();
    std::vector<size_t> rankLogsSizes = m_Comm.GatherValues(rankLogSize);

    // Gatherv JSON per rank
    std::vector<char> profilingJSON(3);
    const std::string header("[\n");
    const std::string footer("\n]\n");
    size_t gatheredSize = 0;
    size_t position = 0;

    if (m_RankMPI == 0) // pre-allocate in destination
    {
        gatheredSize = std::accumulate(rankLogsSizes.begin(),
                                       rankLogsSizes.end(), size_t(0));

        profilingJSON.resize(gatheredSize + header.size() + footer.size() - 2);
        adios2::helper::CopyToBuffer(profilingJSON, position, header.c_str(),
                                     header.size());
    }

    m_Comm.GathervArrays(rankLog.c_str(), rankLog.size(), rankLogsSizes.data(),
                         rankLogsSizes.size(), &profilingJSON[position]);

    if (m_RankMPI == 0) // add footer to close JSON
    {
        position += gatheredSize - 2;
        helper::CopyToBuffer(profilingJSON, position, footer.c_str(),
                             footer.size());
    }

    return profilingJSON;
}

} // end namespace profiling
} // end namespace adios2
