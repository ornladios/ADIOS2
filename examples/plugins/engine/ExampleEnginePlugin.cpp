/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ExampleEnginePlugin.cpp This plugin does nothing but write API calls out to a
 * log file.
 *
 *  Created on: Jul 31, 2017
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 */

#include "ExampleEnginePlugin.h"

#include <cstdio>
#include <cstring>
#include <ctime>

#ifndef _WIN32
#include <sys/time.h>
#endif

namespace
{
std::string now()
{
    tm *timeInfo;
#ifdef _WIN32
    time_t rawTime;
    std::time(&rawTime);
    timeInfo = std::localtime(&rawTime);
#else
    timeval curTime;
    gettimeofday(&curTime, nullptr);
    timeInfo = std::localtime(&curTime.tv_sec);
#endif

    char timeBuf[80];
    std::memset(timeBuf, 0, sizeof(timeBuf));
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%S", timeInfo);

#ifdef _WIN32
    return std::string(timeBuf);
#else
    double subSec = curTime.tv_usec / 1e6;
    char subSecBuf[9];
    std::memset(subSecBuf, 0, sizeof(subSecBuf));
    std::sprintf(subSecBuf, "%1.6f", subSec);
    return std::string(timeBuf) + std::string(&subSecBuf[1]);
#endif
}
}

namespace adios2
{
namespace core
{

namespace engine
{

ExampleEnginePlugin::ExampleEnginePlugin(IO &io, const std::string &name,
                                         const Mode mode, MPI_Comm mpiComm)
: PluginEngineInterface(io, name, mode, mpiComm)
{
    Init();
}

ExampleEnginePlugin::~ExampleEnginePlugin() { m_Log.close(); }

void ExampleEnginePlugin::Init()
{
    std::string logName = "ExamplePlugin.log";
    auto paramLogNameIt = m_IO.m_Parameters.find("LogName");
    if (paramLogNameIt != m_IO.m_Parameters.end())
    {
        logName = paramLogNameIt->second;
    }

    m_Log.open(logName);
    if (!m_Log)
    {
        throw std::ios_base::failure(
            "ExampleEnginePlugin: Failed to open log file " + logName);
    }

    m_Log << now() << " Init" << std::endl;
}

#define define(T)                                                              \
    void ExampleEnginePlugin::DoPutSync(Variable<T> &variable,                 \
                                        const T *values)                       \
    {                                                                          \
        m_Log << now() << " Writing variable \"" << variable.m_Name << "\""    \
              << std::endl;                                                    \
    }
ADIOS2_FOREACH_TYPE_1ARG(define)
#undef define

void ExampleEnginePlugin::DoClose(const int transportIndex)
{
    m_Log << now() << " Close with transportIndex " << transportIndex
          << std::endl;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
