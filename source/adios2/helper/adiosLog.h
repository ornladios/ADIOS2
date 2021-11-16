/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosLog.h
 *
 *  Created on: Nov 15, 2021
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#include <string>

namespace adios2
{
namespace helper
{

enum LogMode : char
{
    EXCEPTION = 'x',
    ERROR = 'e',
    WARNING = 'w',
    OUTPUT = 'o'
};

void Log(const std::string &component, const std::string &source,
         const std::string &operation, const std::string &message,
         const int logRank, const int commRank, const int priority,
         const int verbosity, const LogMode mode);

} // end namespace helper
} // end namespace adios2
