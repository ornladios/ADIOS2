/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosLog.cpp
 *
 *  Created on: Nov 15, 2021
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#include "adiosLog.h"
#include <chrono>
#include <iostream>
#include <sstream>

namespace adios2
{
namespace helper
{
std::string lastEngine;
std::string lastActivity;
std::string lastMessage;
int lastRank;

std::string timeColor = "\033[1;36m";
std::string outputColor = "\033[1;32m";
std::string warningColor = "\033[1;33m";
std::string errorColor = "\033[1;31m";
std::string exceptionColor = "\033[1;34m";
std::string defaultColor = "\033[0m";

void Log(const std::string &component, const std::string &source,
         const std::string &activity, const std::string &message,
         const int logRank, const int commRank, const int priority,
         const int verbosity, const LogMode mode)
{

    if (component == "Engine")
    {
        lastEngine = source;
        lastActivity = activity;
        lastMessage = message;
        lastRank = commRank;
    }

    if (logRank >= 0 && commRank >= 0 && logRank != commRank)
    {
        return;
    }

    if (priority > verbosity)
    {
        return;
    }

    std::stringstream m;

    auto timeNow =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string timeStr(std::ctime(&timeNow));
    if (timeStr[timeStr.size() - 1] == '\n')
    {
        timeStr[timeStr.size() - 1] = '\0';
    }
    m << timeColor << " [" << timeStr << "]" << defaultColor;

    if (mode == OUTPUT)
    {
        m << outputColor << " [ADIOS2 Output]" << defaultColor;
    }
    else if (mode == WARNING)
    {
        m << warningColor << " [ADIOS2 Warning]" << defaultColor;
    }
    else if (mode == ERROR)
    {
        m << errorColor << " [ADIOS2 Error]" << defaultColor;
    }
    else if (mode == EXCEPTION)
    {
        m << exceptionColor << " [ADIOS2 Exception]" << defaultColor;
    }

    if (commRank >= 0)
    {
        m << " [Rank " << commRank << "]";
    }

    m << " " << component << " " << source << " " << activity << ": "
      << message;

    if (mode == OUTPUT || mode == WARNING)
    {
        std::cout << m.str() << std::endl;
    }
    else if (mode == ERROR)
    {
        std::cerr << m.str() << std::endl;
    }
    else if (mode == EXCEPTION)
    {
        std::cerr << m.str() << std::endl;
        throw(m.str());
    }
}

} // end namespace helper
} // end namespace adios2
