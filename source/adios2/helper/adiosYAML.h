/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosYAML.h basic YAML parsing functionality for ADIOS config file schema
 *
 *  Created on: Oct 24, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSYAML_H_
#define ADIOS2_HELPER_ADIOSYAML_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <deque>
#include <map>
#include <memory> //std::shared_ptr
#include <string>
#include <utility> //std::pair
/// \endcond

#include "adios2/common/ADIOSTypes.h" // UserOptions
#include "adios2/core/ADIOS.h"
#include "adios2/core/IO.h"

namespace adios2
{
namespace helper
{

void ParseConfigYAMLIO(core::ADIOS &adios, const std::string &configFileYAML,
                       const std::string &configFileContents, core::IO &io);

std::string ParseConfigYAML(core::ADIOS &adios, const std::string &configFileYAML,
                            std::map<std::string, core::IO> &ios);

void ParseUserOptionsFile(Comm &comm, const std::string &configFileYAML, UserOptions &options,
                          std::string &homePath);

void ParseHostOptionsFile(Comm &comm, const std::string &configFileYAML, HostOptions &hosts,
                          std::string &homePath);

// ATS file format support
// <string-local-path>:
//     remotehost: <string>
//     remotepath: <string>
//     uuid:  <uuid-string>
// all sub-entries are optional, non-existent for local files
// special name for termination of stream:  --end--:
struct TimeSeriesEntry
{
    std::string localpath = "";
    std::string remotehost = "";
    std::string remotepath = "";
    std::string uuid = "";
};

/** TimeSeriesList holds the list of files from in an ATS file */
struct TimeSeriesList
{
    int lastUsedEntry = -1;
    bool ended = false;
    std::deque<TimeSeriesEntry> entries;
};

void ParseTimeSeriesFile(Comm &comm, const std::string &atsYAML, TimeSeriesList &tsl);

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSXML_H_ */
