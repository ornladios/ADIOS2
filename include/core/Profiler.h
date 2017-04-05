/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Profiler.h
 *
 *  Created on: Mar 9, 2017
 *      Author: wfg
 */

#ifndef PROFILER_H_
#define PROFILER_H_

#include <chrono>

#include "Support.h"

namespace adios
{

class Timer
{

public:
    const std::string Process;
    unsigned long long int ProcessTime = 0;

    Timer(const std::string process, const Support::Resolutions resolution)
    : Process{process}, Resolution{resolution}
    {
    }

    void SetInitialTime()
    {
        InitialTime = std::chrono::high_resolution_clock::now();
    }

    void SetTime()
    {
        ElapsedTime = std::chrono::high_resolution_clock::now();
        ProcessTime += GetTime();
    }

    long long int GetTime()
    {
        if (Resolution == Support::Resolutions::mus)
            return std::chrono::duration_cast<std::chrono::microseconds>(
                       ElapsedTime - InitialTime)
                .count();

        else if (Resolution == Support::Resolutions::ms)
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       ElapsedTime - InitialTime)
                .count();

        else if (Resolution == Support::Resolutions::s)
            return std::chrono::duration_cast<std::chrono::seconds>(
                       ElapsedTime - InitialTime)
                .count();

        else if (Resolution == Support::Resolutions::m)
            return std::chrono::duration_cast<std::chrono::minutes>(
                       ElapsedTime - InitialTime)
                .count();

        else if (Resolution == Support::Resolutions::h)
            return std::chrono::duration_cast<std::chrono::hours>(ElapsedTime -
                                                                  InitialTime)
                .count();

        return -1; // failure
    }

    std::string GetUnits() const
    {
        std::string units;
        if (Resolution == Support::Resolutions::mus)
            units = "mus";
        else if (Resolution == Support::Resolutions::ms)
            units = "ms";
        else if (Resolution == Support::Resolutions::s)
            units = "s";
        else if (Resolution == Support::Resolutions::m)
            units = "m";
        else if (Resolution == Support::Resolutions::h)
            units = "h";
        return units;
    }

private:
    const Support::Resolutions Resolution;
    std::chrono::time_point<std::chrono::high_resolution_clock> InitialTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> ElapsedTime;
    bool InitialTimeSet = false;
};

/**
 * Utilities for profiling using the chrono header in C++11
 */
struct Profiler
{
    std::vector<Timer> m_Timers;
    std::vector<unsigned long long int> m_TotalBytes;
    bool m_IsActive = false;
};

} // end namespace

#endif /* PROFILER_H_ */
