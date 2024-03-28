/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */

#include "adiosINI.h"

#include <INIReader.h> // thirdparty inih/cpp

namespace adios2
{
namespace helper
{

inline void FixHomePath(std::string &path, std::string &homePath)
{
    if (!path.empty() && path[0] == '~')
    {
        path = homePath + path.substr(1);
    }
}

void ParseINIFile(Comm &comm, const std::string &configFileINI, INIOptions &options,
                  std::string &homePath)
{
    const std::string hint =
        "when parsing user config file " + configFileINI + " in call to ADIOS constructor";

    const std::string configFileContents = comm.BroadcastFile(configFileINI, hint);
    INIReader iniReader(configFileContents.c_str(), configFileContents.size());

    int err = iniReader.ParseError();
    if (err)
    {
        helper::Throw<std::invalid_argument>("Helper", "adiosINI", "ParseINIFile",
                                             "parser error in file " + configFileINI + " at line " +
                                                 std::to_string(err) + hint);
    }

    /*
     * This code section below determines what options we recognize at all from the
     * ~/.config/adios2/adios2.ini file
     */

    if (iniReader.HasSection("General"))
    {
        options.general.verbose = static_cast<int>(iniReader.GetInteger("General", "verbose", 0));
    }

    if (iniReader.HasSection("Campaign"))
    {
        options.campaign.active = iniReader.GetBoolean("Campaign", "active", true);
        options.campaign.verbose = static_cast<int>(iniReader.GetInteger("Campaign", "verbose", 0));
        options.campaign.hostname = iniReader.Get("Campaign", "hostname", "");
        options.campaign.campaignstorepath = iniReader.Get("Campaign", "campaignstorepath", "");
        FixHomePath(options.campaign.campaignstorepath, homePath);
        options.campaign.cachepath = iniReader.Get("Campaign", "cachepath", "/tmp/adios2-cache");
        FixHomePath(options.campaign.cachepath, homePath);
    }

    if (iniReader.HasSection("SST"))
    {
        options.sst.verbose = static_cast<int>(iniReader.GetInteger("SST", "verbose", 0));
    }
}

} // end namespace helper
} // end namespace adios2
