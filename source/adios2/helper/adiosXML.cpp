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
#include "adios2/helper/adiosXMLUtil.h"

#include <pugixml.hpp>

namespace adios2
{
namespace helper
{

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

        if (fileContents.empty())
        {
            throw std::invalid_argument("ERROR: config xml file is empty, " +
                                        hint + "\n");
        }
        return fileContents;
    };

    auto lf_OperatorXML = [&](const pugi::xml_node &operatorNode) {
        const std::unique_ptr<pugi::xml_attribute> name =
            helper::XMLAttribute("name", operatorNode, hint);

        const std::unique_ptr<pugi::xml_attribute> type =
            helper::XMLAttribute("type", operatorNode, hint);

        std::string typeLowerCase = std::string(type->value());
        std::transform(typeLowerCase.begin(), typeLowerCase.end(),
                       typeLowerCase.begin(), ::tolower);

        const Params parameters = helper::XMLGetParameters(operatorNode, hint);

        adios.DefineOperator(name->value(), typeLowerCase, parameters);
    };

    // node is the variable node
    auto lf_IOVariableXML = [&](const pugi::xml_node &node,
                                core::IO &currentIO) {
        const std::string variableName =
            std::string(helper::XMLAttribute("name", node, hint)->value());

        for (const pugi::xml_node &operation : node.children("operation"))
        {
            const std::unique_ptr<pugi::xml_attribute> opName =
                helper::XMLAttribute("operator", operation, hint, false);

            const std::unique_ptr<pugi::xml_attribute> opType =
                helper::XMLAttribute("type", operation, hint, false);

            if (*opName && *opType)
            {
                throw std::invalid_argument(
                    "ERROR: operator (" + std::string(opName->value()) +
                    ") and type (" + std::string(opType->value()) +
                    ") attributes can't coexist in <operation> element "
                    "inside <variable name=\"" +
                    variableName + "\"> element, " + hint + "\n");
            }

            if (!*opName && !*opType)
            {
                throw std::invalid_argument(
                    "ERROR: <operation> element "
                    "inside <variable name=\"" +
                    variableName +
                    "\"> element requires either operator "
                    "(existing) or type (supported) attribute, " +
                    hint + "\n");
            }

            core::Operator *op = nullptr;

            if (*opName)
            {
                auto itOperator = operators.find(std::string(opName->value()));
                if (itOperator == operators.end())
                {
                    throw std::invalid_argument(
                        "ERROR: operator " + std::string(opName->value()) +
                        " not previously defined, from variable " +
                        variableName + " inside io " + currentIO.m_Name + ", " +
                        hint + "\n");
                }
                op = itOperator->second.get();
            }

            if (*opType)
            {
                std::string operatorType = std::string(opType->value());
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
            const Params parameters = helper::XMLGetParameters(operation, hint);
            currentIO.m_VarOpsPlaceholder[variableName].push_back(
                core::IO::Operation{op, parameters, Params()});
        }
    };

    auto lf_IOXML = [&](const pugi::xml_node &io) {
        const std::unique_ptr<pugi::xml_attribute> ioName =
            helper::XMLAttribute("name", io, hint);

        // Build the IO object
        auto itCurrentIO =
            ios.emplace(ioName->value(), core::IO(adios, ioName->value(), true,
                                                  adios.m_HostLanguage));
        core::IO &currentIO = itCurrentIO.first->second;

        // must be unique per io
        const std::unique_ptr<pugi::xml_node> engine =
            helper::XMLNode("engine", io, hint, false, true);

        if (*engine)
        {
            const std::unique_ptr<pugi::xml_attribute> type =
                helper::XMLAttribute("type", *engine, hint);
            currentIO.SetEngine(type->value());

            const Params parameters = helper::XMLGetParameters(*engine, hint);
            currentIO.SetParameters(parameters);
        }

        for (const pugi::xml_node &transport : io.children("transport"))
        {
            const std::unique_ptr<pugi::xml_attribute> type =
                helper::XMLAttribute("type", transport, hint);

            const Params parameters = helper::XMLGetParameters(transport, hint);
            currentIO.AddTransport(type->value(), parameters);
        }

        for (const pugi::xml_node &variable : io.children("variable"))
        {
            lf_IOVariableXML(variable, currentIO);
        }
    };

    // BODY OF FUNCTION
    const std::string fileContents = lf_FileContents(configFileXML);
    const std::unique_ptr<pugi::xml_document> document =
        helper::XMLDocument(fileContents, hint);

    // must be unique
    const std::unique_ptr<pugi::xml_node> config =
        helper::XMLNode("adios-config", *document, hint, true);

    for (const pugi::xml_node &op : config->children("operator"))
    {
        lf_OperatorXML(op);
    }

    for (const pugi::xml_node &io : config->children("io"))
    {
        lf_IOXML(io);
    }
}

} // end namespace helper
} // end namespace adios2
