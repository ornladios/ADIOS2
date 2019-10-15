/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosXML.cpp
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 *              Chuck Atkins chuck.atkins@kitware.com
 */

#include "adiosXML.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::transform
#include <iterator>  // std::distance
#include <stdexcept> //std::invalid_argument
/// \endcond

#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosString.h"

#include <pugixml.hpp>

namespace adios2
{
namespace helper
{

namespace
{
pugi::xml_document XMLDocument(const std::string &xmlContents,
                               const bool debugMode, const std::string hint)
{
    pugi::xml_document document;
    auto parse_result = document.load_buffer_inplace(
        const_cast<char *>(xmlContents.data()), xmlContents.size());

    if (debugMode)
    {
        if (!parse_result)
        {
            throw std::invalid_argument(
                "ERROR: XML: parse error in XML string, description: " +
                std::string(parse_result.description()) +
                ", check with any XML editor if format is ill-formed, " + hint +
                "\n");
        }
    }
    return document;
}

pugi::xml_node XMLNode(const std::string nodeName,
                       const pugi::xml_document &xmlDocument,
                       const bool debugMode, const std::string hint,
                       const bool isMandatory = true,
                       const bool isUnique = false)
{
    const pugi::xml_node node = xmlDocument.child(nodeName.c_str());

    if (debugMode)
    {
        if (isMandatory && !node)
        {
            throw std::invalid_argument("ERROR: XML: no <" + nodeName +
                                        "> element found, " + hint);
        }

        if (isUnique)
        {
            const size_t nodes =
                std::distance(xmlDocument.children(nodeName.c_str()).begin(),
                              xmlDocument.children(nodeName.c_str()).end());
            if (nodes > 1)
            {
                throw std::invalid_argument("ERROR: XML only one <" + nodeName +
                                            "> element can exist inside " +
                                            std::string(xmlDocument.name()) +
                                            ", " + hint + "\n");
            }
        }
    }
    return node;
}

pugi::xml_node XMLNode(const std::string nodeName,
                       const pugi::xml_node &upperNode, const bool debugMode,
                       const std::string hint, const bool isMandatory = true,
                       const bool isUnique = false)
{
    const pugi::xml_node node = upperNode.child(nodeName.c_str());

    if (debugMode)
    {
        if (isMandatory && !node)
        {
            throw std::invalid_argument(
                "ERROR: XML: no <" + nodeName + "> element found, inside <" +
                std::string(upperNode.name()) + "> element " + hint);
        }

        if (isUnique)
        {
            const size_t nodes =
                std::distance(upperNode.children(nodeName.c_str()).begin(),
                              upperNode.children(nodeName.c_str()).end());
            if (nodes > 1)
            {
                throw std::invalid_argument("ERROR: XML only one <" + nodeName +
                                            "> element can exist inside <" +
                                            std::string(upperNode.name()) +
                                            "> element, " + hint + "\n");
            }
        }
    }
    return node;
}

pugi::xml_attribute XMLAttribute(const std::string attributeName,
                                 const pugi::xml_node &node,
                                 const bool debugMode, const std::string hint,
                                 const bool isMandatory = true)
{
    const pugi::xml_attribute attribute = node.attribute(attributeName.c_str());

    if (debugMode)
    {
        if (isMandatory && !attribute)
        {
            const std::string nodeName(node.name());

            throw std::invalid_argument("ERROR: XML: No attribute " +
                                        attributeName + " found on <" +
                                        nodeName + "> element" + hint);
        }
    }
    return attribute;
}
} // end empty namespace

void ParseConfigXML(
    core::ADIOS &adios, const std::string &configFileXML,
    std::map<std::string, core::IO> &ios,
    std::map<std::string, std::shared_ptr<core::Operator>> &operators)
{
    const std::string hint("for config file " + configFileXML +
                           " in call to ADIOS constructor");

    auto lf_FileContents = [&](const std::string &configXML) -> std::string {
        const std::string fileContents(adios.GetComm().BroadcastFile(
            configXML,
            "when parsing configXML file, in call to the ADIOS constructor"));

        if (adios.m_DebugMode)
        {
            if (fileContents.empty())
            {
                throw std::invalid_argument(
                    "ERROR: config xml file is empty, " + hint + "\n");
            }
        }
        return fileContents;
    };

    auto lf_GetParametersXML = [&](const pugi::xml_node &node) -> Params {
        const std::string errorMessage("in node " + std::string(node.value()) +
                                       ", " + hint);
        Params parameters;

        for (const pugi::xml_node paramNode : node.children("parameter"))
        {
            const pugi::xml_attribute key =
                XMLAttribute("key", paramNode, adios.m_DebugMode, errorMessage);

            const pugi::xml_attribute value = helper::XMLAttribute(
                "value", paramNode, adios.m_DebugMode, errorMessage);

            parameters.emplace(key.value(), value.value());
        }
        return parameters;
    };

    auto lf_OperatorXML = [&](const pugi::xml_node &operatorNode) {
        const pugi::xml_attribute name =
            helper::XMLAttribute("name", operatorNode, adios.m_DebugMode, hint);

        const pugi::xml_attribute type =
            helper::XMLAttribute("type", operatorNode, adios.m_DebugMode, hint);

        std::string typeLowerCase = std::string(type.value());
        std::transform(typeLowerCase.begin(), typeLowerCase.end(),
                       typeLowerCase.begin(), ::tolower);

        const Params parameters = lf_GetParametersXML(operatorNode);

        adios.DefineOperator(name.value(), typeLowerCase, parameters);
    };

    // node is the variable node
    auto lf_IOVariableXML = [&](const pugi::xml_node &node,
                                core::IO &currentIO) {
        const std::string variableName = std::string(
            helper::XMLAttribute("name", node, adios.m_DebugMode, hint)
                .value());

        for (const pugi::xml_node &operation : node.children("operation"))
        {
            const pugi::xml_attribute opName = helper::XMLAttribute(
                "operator", operation, adios.m_DebugMode, hint, false);

            const pugi::xml_attribute opType = helper::XMLAttribute(
                "type", operation, adios.m_DebugMode, hint, false);

            if (adios.m_DebugMode)
            {
                if (opName && opType)
                {
                    throw std::invalid_argument(
                        "ERROR: operator (" + std::string(opName.value()) +
                        ") and type (" + std::string(opType.value()) +
                        ") attributes can't coexist in <operation> element "
                        "inside <variable name=\"" +
                        variableName + "\"> element, " + hint + "\n");
                }

                if (!opName && !opType)
                {
                    throw std::invalid_argument(
                        "ERROR: <operation> element "
                        "inside <variable name=\"" +
                        variableName +
                        "\"> element requires either operator "
                        "(existing) or type (supported) attribute, " +
                        hint + "\n");
                }
            }

            core::Operator *op = nullptr;

            if (opName)
            {
                auto itOperator = operators.find(std::string(opName.value()));
                if (adios.m_DebugMode)
                {
                    if (itOperator == operators.end())
                    {
                        throw std::invalid_argument(
                            "ERROR: operator " + std::string(opName.value()) +
                            " not previously defined, from variable " +
                            variableName + " inside io " + currentIO.m_Name +
                            ", " + hint + "\n");
                    }
                }
                op = itOperator->second.get();
            }

            if (opType)
            {
                std::string operatorType = std::string(opType.value());
                std::transform(operatorType.begin(), operatorType.end(),
                               operatorType.begin(), ::tolower);

                const std::string operatorName =
                    "__" + currentIO.m_Name + "_" + operatorType;
                auto itOperator = operators.find(operatorName);

                if (itOperator == operators.end())
                {
                    op = &adios.DefineOperator(operatorName, operatorType);
                }
                else
                {
                    op = itOperator->second.get();
                }
            }
            const Params parameters = lf_GetParametersXML(operation);
            currentIO.m_VarOpsPlaceholder[variableName].push_back(
                core::IO::Operation{op, parameters, Params()});
        }
    };

    auto lf_IOXML = [&](const pugi::xml_node &io) {
        const pugi::xml_attribute ioName =
            helper::XMLAttribute("name", io, adios.m_DebugMode, hint);

        // Build the IO object
        auto itCurrentIO = ios.emplace(
            ioName.value(), core::IO(adios, ioName.value(), true,
                                     adios.m_HostLanguage, adios.m_DebugMode));
        core::IO &currentIO = itCurrentIO.first->second;

        // must be unique per io
        const pugi::xml_node &engine =
            helper::XMLNode("engine", io, adios.m_DebugMode, hint, false, true);

        if (engine)
        {
            const pugi::xml_attribute type =
                helper::XMLAttribute("type", engine, adios.m_DebugMode, hint);
            currentIO.SetEngine(type.value());

            const Params parameters = lf_GetParametersXML(engine);
            currentIO.SetParameters(parameters);
        }

        for (const pugi::xml_node &variable : io.children("variable"))
        {
            lf_IOVariableXML(variable, currentIO);
        }
    };

    // BODY OF FUNCTION
    const std::string fileContents = lf_FileContents(configFileXML);
    const pugi::xml_document document =
        helper::XMLDocument(fileContents, adios.m_DebugMode, hint);

    // must be unique
    const pugi::xml_node config = helper::XMLNode(
        "adios-config", document, adios.m_DebugMode, hint, true);

    for (const pugi::xml_node &op : config.children("operator"))
    {
        lf_OperatorXML(op);
    }

    for (const pugi::xml_node &io : config.children("io"))
    {
        lf_IOXML(io);
    }
}

} // end namespace helper
} // end namespace adios2
