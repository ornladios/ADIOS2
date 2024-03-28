/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */

#include "adiosUserOptions.h"

#include "adios2/helper/adiosString.h"

#include <yaml-cpp/yaml.h>

namespace adios2
{
namespace helper
{

namespace
{

inline void FixHomePath(std::string &path, std::string &homePath)
{
    if (!path.empty() && path[0] == '~')
    {
        path = homePath + path.substr(1);
    }
}

constexpr bool isMandatory = true;
constexpr bool isNotMandatory = false;

YAML::Node YAMLNode(const std::string nodeName, const YAML::Node &upperNode,
                    const std::string &hint, const bool isMandatory,
                    const YAML::NodeType::value nodeType)
{
    const YAML::Node node = upperNode[nodeName];

    if (isMandatory && !node)
    {
        helper::Throw<std::invalid_argument>(
            "Helper", "adiosYAML", "YAMLNode",
            "no " + nodeName + " node found, (is your node key lower case?), " + hint);
    }
    if (node && node.Type() != nodeType)
    {
        helper::Throw<std::invalid_argument>("Helper", "adiosYAML", "YAMLNode",
                                             "node " + nodeName +
                                                 " is the wrong type, review adios2 "
                                                 "config YAML specs for the node, " +
                                                 hint);
    }
    return node;
}

} // end empty  namespace

void ParseUserOptionsFile(Comm &comm, const std::string &configFileYAML, UserOptions &options,
                          std::string &homePath)
{
    const std::string hint =
        "when parsing user config file " + configFileYAML + " in call to ADIOS constructor";

    const std::string configFileContents = comm.BroadcastFile(configFileYAML, hint);
    const YAML::Node document = YAML::Load(configFileContents);

    if (!document)
    {
        helper::Throw<std::invalid_argument>(
            "Helper", "adiosUserOptions", "ParseUserOptionsFile",
            "parser error in file " + configFileYAML +
                " invalid format. Check with any YAML editor if format is ill-formed, " + hint);
    }

    /*
     * This code section below determines what options we recognize at all from the
     * ~/.config/adios2/adios2.yaml file
     */

    for (auto itNode = document.begin(); itNode != document.end(); ++itNode)
    {
        std::cout << itNode->const YAML::Node general =
            YAMLNode("IO", *itNode, hint, isNotMandatory, YAML::NodeType::Scalar);
        if (ioScalar)
        {
            const std::string ioName = ioScalar.as<std::string>();
            // Build the IO object
            auto itCurrentIO =
                ios.emplace(std::piecewise_construct, std::forward_as_tuple(ioName),
                            std::forward_as_tuple(adios, ioName, true, adios.m_HostLanguage));
            core::IO &currentIO = itCurrentIO.first->second;
            IOYAML(adios, *itNode, currentIO, hint);
        }
    }

    /*
        if (iniReader.HasSection("General"))
        {
            options.general.verbose = static_cast<int>(iniReader.GetInteger("General", "verbose",
       0));
        }

        if (iniReader.HasSection("Campaign"))
        {
            options.campaign.active = iniReader.GetBoolean("Campaign", "active", true);
            options.campaign.verbose = static_cast<int>(iniReader.GetInteger("Campaign", "verbose",
       0)); options.campaign.hostname = iniReader.Get("Campaign", "hostname", "");
            options.campaign.campaignstorepath = iniReader.Get("Campaign", "campaignstorepath", "");
            FixHomePath(options.campaign.campaignstorepath, homePath);
            options.campaign.cachepath = iniReader.Get("Campaign", "cachepath",
       "/tmp/adios2-cache"); FixHomePath(options.campaign.cachepath, homePath);
        }

        if (iniReader.HasSection("SST"))
        {
            options.sst.verbose = static_cast<int>(iniReader.GetInteger("SST", "verbose", 0));
        }
    */
}

} // end namespace helper
} // end namespace adios2
