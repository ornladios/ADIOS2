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
#include <stdexcept> //std::invalid_argument
/// \endcond

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/helper/adiosMPIFunctions.h"
#include "adios2/helper/adiosString.h"

namespace adios2
{
namespace helper
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
                "ERROR: XML: parse error in string " + xmlContents +
                " description: " + std::string(parse_result.description()) +
                ", check with any XML editor if format is ill-formed, " + hint +
                "\n");
        }
    }
    return document;
}

pugi::xml_node XMLNode(const std::string nodeName,
                       const pugi::xml_document &xmlDocument,
                       const bool debugMode, const std::string hint,
                       const bool isUnique)
{
    const pugi::xml_node node = xmlDocument.child(nodeName.c_str());

    if (debugMode)
    {
        if (!node)
        {
            throw std::invalid_argument("ERROR: XML: no <" + nodeName +
                                        "> element found, " + hint);
        }

        if (isUnique)
        {
            const pugi::xml_node node2 = xmlDocument.child(nodeName.c_str());
            if (node2)
            {
                throw std::invalid_argument("ERROR: XML only one <" + nodeName +
                                            "> element can exist inside " +
                                            xmlDocument.name() + ", " + hint +
                                            "\n");
            }
        }
    }
    return node;
}

pugi::xml_node XMLNode(const std::string nodeName,
                       const pugi::xml_node &upperNode, const bool debugMode,
                       const std::string hint, const bool isUnique)
{
    const pugi::xml_node node = upperNode.child(nodeName.c_str());

    if (debugMode)
    {
        if (!node)
        {
            throw std::invalid_argument("ERROR: XML: no <" + nodeName +
                                        "> element found, inside <" +
                                        upperNode.name() + "> element " + hint);
        }

        if (isUnique)
        {
            const pugi::xml_node node2 = upperNode.child(nodeName.c_str());
            if (node2)
            {
                throw std::invalid_argument("ERROR: XML only one <" + nodeName +
                                            "> element can exist inside <" +
                                            upperNode.name() + "> element, " +
                                            hint + "\n");
            }
        }
    }
    return node;
}

pugi::xml_attribute XMLAttribute(const std::string attributeName,
                                 const pugi::xml_node &node,
                                 const bool debugMode, const std::string hint)
{
    const pugi::xml_attribute attribute = node.attribute(attributeName.c_str());

    if (debugMode)
    {
        if (!attribute)
        {
            const std::string nodeName(node.name());

            throw std::invalid_argument("ERROR: XML: No attribute " +
                                        attributeName + " found on <" +
                                        nodeName + "> element" + hint);
        }
    }
    return attribute;
}

} // end namespace helper
} // end namespace adios2
