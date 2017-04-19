/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Support.h
 *
 *  Created on: Oct 10, 2016
 *      Author: wfg
 */

#ifndef ADIOS2_CORE_SUPPORT_H_
#define ADIOS2_CORE_SUPPORT_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <set>
#include <string>
/// \endcond

#include "adios2/ADIOSConfig.h"

namespace adios
{

struct Support
{
    ///< current ADIOS version
    static const std::string Version;

    ///< supported languages: C, C++, Fortran, Python, Java
    static const std::set<std::string> HostLanguages;

    static const std::set<std::string> Numbers;

    ///< supported transport methods
    static const std::set<std::string> Transports;

    ///< supported data transform methods
    static const std::set<std::string> Transforms;

    ///< supported data types, key: host language, value: all supported types
    static const std::map<std::string, std::set<std::string>> Datatypes;

    ///< all supported int aliases, key: C++ type (e.g. int), value: aliases to
    /// type in key (e.g. int, integer)
    static const std::map<std::string, std::set<std::string>> DatatypesAliases;

    ///< file I/O transports
    static const std::set<std::string> FileTransports;

    enum class Resolutions
    {
        mus,
        ms,
        s,
        m,
        h
    };
};

} // end namespace adios

#endif /* ADIOS2_CORE_SUPPORT_H_ */
