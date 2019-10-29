/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosYAML.h basic YAML parsing functionality for ADIOS config file schema
 *
 *  Created on: Oct 24, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosYAML.h"

#include "adios2/helper/adiosString.h"

#include <yaml-cpp/yaml.h>

namespace adios2
{
namespace helper
{

namespace
{

YAML::Node YAMLNode(const std::string nodeName, const YAML::Node &upperNode,
                    const bool debugMode, const std::string &hint,
                    const bool isMandatory,
                    const YAML::NodeType::value nodeType)
{
    const YAML::Node node = upperNode[nodeName];

    if (debugMode)
    {
        if (isMandatory && !node)
        {
            throw std::invalid_argument(
                "ERROR: YAML: no " + nodeName +
                " node found, (is your node key lower case?), " + hint);
        }
        if (node && node.Type() != nodeType)
        {
            throw std::invalid_argument("ERROR: YAML: node " + nodeName +
                                        " is the wrong type, review adios2 "
                                        "config YAML specs for the node, " +
                                        hint);
        }
    }
    return node;
}

Params YAMLNodeMapToParams(const YAML::Node &node, const bool debugMode,
                           const std::string &hint)
{
    Params parameters;
    for (auto itParam = node.begin(); itParam != node.end(); ++itParam)
    {
        const std::string key = itParam->first.as<std::string>();
        const std::string value = itParam->second.as<std::string>();
        auto it = parameters.emplace(key, value);
        if (debugMode && !it.second)
        {
            throw std::invalid_argument(
                "ERROR: YAML: found duplicated key : " + key +
                ", keys must be unique in a YAML node, " + hint);
        }
    }
    return parameters;
}

} // end empty  namespace

void ParseConfigYAML(
    core::ADIOS &adios, const std::string &configFileYAML,
    std::map<std::string, core::IO> &ios,
    std::map<std::string, std::shared_ptr<core::Operator>> &operators)
{
    const std::string hint = "when parsing config file " + configFileYAML +
                             " in call to ADIOS constructor";

    constexpr bool isMandatory = true;
    constexpr bool isNotMandatory = false;

    auto lf_IOVariableYAML = [&](const YAML::Node &variableMap,
                                 core::IO &currentIO) {
        const YAML::Node &variableNameScalar =
            YAMLNode("Variable", variableMap, adios.m_DebugMode, hint,
                     isMandatory, YAML::NodeType::Scalar);
        const std::string variableName = variableNameScalar.as<std::string>();

        const YAML::Node operationsSequence =
            YAMLNode("Operations", variableMap, adios.m_DebugMode, hint,
                     isNotMandatory, YAML::NodeType::Sequence);

        if (operationsSequence)
        {
            // loop through each transport node
            const std::string errorMessage =
                " in operations node from variable " + variableName + ", " +
                hint;

            for (auto it = operationsSequence.begin();
                 it != operationsSequence.end(); ++it)
            {
                const YAML::Node typeScalar =
                    YAMLNode("Type", *it, adios.m_DebugMode, errorMessage,
                             isMandatory, YAML::NodeType::Scalar);

                core::Operator *op = nullptr;

                Params parameters =
                    YAMLNodeMapToParams(*it, adios.m_DebugMode, hint);

                const std::string operatorType =
                    EraseKey<std::string>("Type", parameters);

                const std::string operatorName =
                    "__" + currentIO.m_Name + "_" + operatorType;

                auto itOperator = operators.find(operatorName);
                op = (itOperator == operators.end())
                         ? &adios.DefineOperator(operatorName, operatorType)
                         : itOperator->second.get();

                currentIO.m_VarOpsPlaceholder[variableName].push_back(
                    core::IO::Operation{op, parameters, Params()});
            }
        }
    };

    auto lf_IOYAML = [&](const std::string &ioName, const YAML::Node &ioMap) {
        // Build the IO object
        auto itCurrentIO = ios.emplace(ioName, core::IO(adios, ioName, true,
                                                        adios.m_HostLanguage,
                                                        adios.m_DebugMode));
        core::IO &currentIO = itCurrentIO.first->second;

        // Engine parameters
        const YAML::Node engineMap =
            YAMLNode("Engine", ioMap, adios.m_DebugMode, hint, false,
                     YAML::NodeType::Map);

        if (engineMap)
        {
            Params parameters =
                YAMLNodeMapToParams(engineMap, adios.m_DebugMode, hint);
            auto itType = parameters.find("Type");
            if (itType != parameters.end())
            {
                const std::string type =
                    EraseKey<std::string>("Type", parameters);
                currentIO.SetEngine(type);
            }
            currentIO.SetParameters(parameters);
        }

        // Variables
        const YAML::Node variablesSequence =
            YAMLNode("Variables", ioMap, adios.m_DebugMode, hint, false,
                     YAML::NodeType::Sequence);

        if (variablesSequence)
        {
            // loop through each variable node
            for (const YAML::Node &variableMap : variablesSequence)
            {
                lf_IOVariableYAML(variableMap, currentIO);
            }
        }

        // Transports
        const YAML::Node transportsSequence =
            YAMLNode("Transports", ioMap, adios.m_DebugMode, hint, false,
                     YAML::NodeType::Sequence);

        if (transportsSequence)
        {
            // loop through each transport node
            for (auto it = transportsSequence.begin();
                 it != transportsSequence.end(); ++it)
            {
                if (adios.m_DebugMode)
                {
                    YAMLNode("Type", *it, true, " in transport node " + hint,
                             isMandatory, YAML::NodeType::Scalar);
                }

                Params parameters =
                    YAMLNodeMapToParams(*it, adios.m_DebugMode, hint);
                const std::string type =
                    EraseKey<std::string>("Type", parameters);

                currentIO.AddTransport(type, parameters);
            }
        }
    };

    // BODY OF FUNCTION STARTS HERE
    const std::string fileContents =
        adios.GetComm().BroadcastFile(configFileYAML, hint);
    const YAML::Node document = YAML::Load(fileContents);

    if (adios.m_DebugMode && !document)
    {
        throw std::invalid_argument(
            "ERROR: YAML: parser error in file " + configFileYAML +
            " invalid format check with any YAML editor if format is "
            "ill-formed, " +
            hint + "\n");
    }

    for (auto itNode = document.begin(); itNode != document.end(); ++itNode)
    {
        const YAML::Node ioScalar =
            YAMLNode("IO", *itNode, adios.m_DebugMode, hint, isNotMandatory,
                     YAML::NodeType::Scalar);
        if (ioScalar)
        {
            const std::string ioName = ioScalar.as<std::string>();
            lf_IOYAML(ioName, *itNode);
        }
    }
}

} // end namespace helper
} // end namespace adios2
