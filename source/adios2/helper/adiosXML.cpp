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

#include <pugixml.hpp>

namespace adios2
{

Params InitParametersXML(const pugi::xml_node &node, const bool debugMode)
{
    Params params;
    for (const pugi::xml_node paramNode : node.children("parameter"))
    {
        const pugi::xml_attribute key = paramNode.attribute("key");
        if (debugMode)
        {
            if (!key)
            {
                throw std::invalid_argument("ERROR: XML: No \"key\" attribute "
                                            "found on <parameter> element, in "
                                            "call to ADIOS constructor\n");
            }
        }

        const pugi::xml_attribute value = paramNode.attribute("value");

        if (debugMode)
        {
            if (!value)
            {

                throw std::invalid_argument("ERROR: XML: No \"value\" "
                                            "attribute found on <parameter> "
                                            "element,  for key " +
                                            std::string(key.value()) +
                                            ", in call to ADIOS constructor\n");
            }
        }

        params.emplace(key.value(), value.value());
    }
    return params;
}

void InitIOXML(const pugi::xml_node &ioNode, MPI_Comm mpiComm,
               const std::string hostLanguage, const bool debugMode,
               std::map<std::string, std::shared_ptr<Operator>> &transforms,
               std::map<std::string, IO> &ios)
{
    // Extract <io name=""> attribute
    const pugi::xml_attribute nameAttr = ioNode.attribute("name");
    if (!nameAttr)
    {
        if (debugMode)
        {
            throw std::invalid_argument("ERROR: XML: No \"name\" attribute "
                                        "found on <io> element, in call to "
                                        "ADIOS constructor.\n");
        }
        return;
    }
    const std::string ioName = nameAttr.value();

    // Build the IO object
    auto ioIt =
        ios.emplace(ioName, IO(ioName, mpiComm, true, hostLanguage, debugMode));
    IO &io = ioIt.first->second;

    // Extract <engine> element
    if (debugMode)
    {
        unsigned int count = 0;

        for (const pugi::xml_node engineNode : ioNode.children("engine"))
        {
            ++count;
            if (count == 2)
            {
                throw std::invalid_argument(
                    "ERROR: XML only one <engine> element "
                    "can exist inside an <io> element from io " +
                    ioName + ", in call to ADIOS constructor\n");
            }
        }
    }

    const pugi::xml_node engineNode = ioNode.child("engine");
    if (engineNode)
    {
        const pugi::xml_attribute engineTypeAttr = engineNode.attribute("type");

        if (debugMode)
        {
            if (!engineTypeAttr)
            {
                throw std::invalid_argument(
                    "ERROR: XML: No \"type\" attribute "
                    "found on <engine> element, in call to "
                    "ADIOS constructor");
            }
        }

        io.SetEngine(engineTypeAttr.value());
    }

    // Process <engine> parameters
    io.SetParameters(InitParametersXML(engineNode, debugMode));

    // Extract and process <transport> elements
    for (const pugi::xml_node transportNode : ioNode.children("transport"))
    {
        const pugi::xml_attribute typeXMLAttribute =
            transportNode.attribute("type");

        if (debugMode)
        {
            if (!typeXMLAttribute)
            {

                throw std::invalid_argument("ERROR: XML: No \"type\" attribute "
                                            "found on <transport> element, in "
                                            "call to ADIOS constructor\n");
            }
        }

        io.AddTransport(typeXMLAttribute.value(),
                        InitParametersXML(transportNode, debugMode));
    }
}

void InitXML(const std::string configXML, MPI_Comm mpiComm,
             const std::string hostLanguage, const bool debugMode,
             std::map<std::string, std::shared_ptr<Operator>> &transforms,
             std::map<std::string, IO> &ios)
{
    int mpiRank;
    MPI_Comm_rank(mpiComm, &mpiRank);
    std::string fileContents;

    // Read the file on rank 0 and broadcast it to everybody else
    if (mpiRank == 0)
    {
        fileContents = FileToString(configXML);
    }

    fileContents = BroadcastValue(fileContents, mpiComm);

    pugi::xml_document doc;
    auto parse_result = doc.load_buffer_inplace(
        const_cast<char *>(fileContents.data()), fileContents.size());

    if (debugMode)
    {
        if (!parse_result)
        {
            throw std::invalid_argument(
                "ERROR: XML: parse error in file " + configXML +
                " description: " + std::string(parse_result.description()) +
                ", in call to ADIOS constructor\n");
        }
    }

    const pugi::xml_node configNode = doc.child("adios-config");

    if (debugMode)
    {
        if (!configNode)
        {
            throw std::invalid_argument(
                "ERROR: XML: No <adios-config> element found in file " +
                configXML + ", in call to ADIOS constructor\n");
        }
    }

    for (const pugi::xml_node ioNode : configNode.children("io"))
    {
        InitIOXML(ioNode, mpiComm, hostLanguage, debugMode, transforms, ios);
    }
}

} // end namespace adios2
