/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosXML.cpp
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosXML.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <stdexcept> //std::invalid_argument
/// \endcond

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/helper/adiosString.h"

#include <pugixml.hpp>

namespace adios2
{

Params InitParametersXML(pugi::xml_node node, bool debugMode)
{
    Params params;
    for (pugi::xml_node paramNode : node.children("parameter"))
    {
        pugi::xml_attribute attrKey = paramNode.attribute("key");
        if (!attrKey)
        {
            if (debugMode)
            {
                throw std::invalid_argument("ERROR: XML: No \"key\" attribute "
                                            "found on <parameter> element.");
            }
            continue;
        }

        pugi::xml_attribute attrValue = paramNode.attribute("value");
        if (!attrValue)
        {
            if (debugMode)
            {
                throw std::invalid_argument("ERROR: XML: No \"value\" "
                                            "attribute found on <parameter> "
                                            "element.");
            }
            continue;
        }

        params.emplace(attrKey.value(), attrValue.value());
    }
    return params;
}

void InitIOXML(const pugi::xml_node ioNode, const MPI_Comm mpiComm,
               const bool debugMode,
               std::vector<std::shared_ptr<Transform>> &transforms,
               std::map<std::string, IO> &ios)
{
    // Extract <io name=""> attribute
    pugi::xml_attribute nameAttr = ioNode.attribute("name");
    if (!nameAttr)
    {
        if (debugMode)
        {
            throw std::invalid_argument(
                "ERROR: XML: No \"name\" attribute found on <io> element.");
        }
        return;
    }
    std::string ioName = nameAttr.value();

    // Build the IO object
    auto ioIt = ios.emplace(ioName, IO(ioName, mpiComm, true, debugMode));
    IO &io = ioIt.first->second;

    // Extract <engine> element
    pugi::xml_node engineNode = ioNode.child("engine");
    if (!engineNode)
    {
        throw std::invalid_argument(
            "ERROR: XML: No <engine> element found in <io> element.");
    }
    pugi::xml_attribute engineTypeAttr = engineNode.attribute("type");
    if (!engineTypeAttr)
    {
        throw std::invalid_argument(
            "ERROR: XML: No \"type\" attribute found on <engine> element.");
    }
    io.SetEngine(engineTypeAttr.value());

    // Process <engine> parameters
    io.SetParameters(InitParametersXML(engineNode, debugMode));

    // Extract and process <transport> elements
    for (pugi::xml_node transportNode : ioNode.children("transport"))
    {
        pugi::xml_attribute typeAttr = transportNode.attribute("type");
        if (!typeAttr)
        {
            if (debugMode)
            {
                throw std::invalid_argument("ERROR: XML: No \"type\" attribute "
                                            "found on <transport> element.");
            }
            continue;
        }
        io.AddTransport(typeAttr.value(),
                        InitParametersXML(transportNode, debugMode));
    }
}

void InitXML(const std::string configXML, const MPI_Comm mpiComm,
             const bool debugMode,
             std::vector<std::shared_ptr<Transform>> &transforms,
             std::map<std::string, IO> &ios)
{
    int mpiRank;
    MPI_Comm_rank(mpiComm, &mpiRank);
    std::string fileContents;
    unsigned long long len;

    // Read the file on rank 0 and broadcast it to everybody else
    if (mpiRank == 0)
    {
        fileContents = FileToString(configXML);
        len = static_cast<unsigned long long>(fileContents.size());
    }
    MPI_Bcast(&len, 1, MPI_UNSIGNED_LONG, 0, mpiComm);
    if (mpiRank != 0)
    {
        fileContents.resize(len);
    }
    MPI_Bcast(const_cast<char *>(fileContents.data()), len, MPI_CHAR, 0,
              mpiComm);

    pugi::xml_document doc;
    auto parse_result = doc.load_buffer_inplace(
        const_cast<char *>(fileContents.data()), fileContents.size());
    if (!parse_result)
    {
        if (debugMode)
        {
            throw std::invalid_argument(
                std::string("ERROR: XML: Parse error: ") +
                parse_result.description());
        }
        return;
    }

    pugi::xml_node configNode = doc.child("adios-config");
    if (!configNode)
    {
        if (debugMode)
        {
            throw std::invalid_argument(
                "ERROR: XML: No <adios-config> element found");
        }
        return;
    }

    ios.clear();
    for (pugi::xml_node ioNode : configNode.children("io"))
    {
        InitIOXML(ioNode, mpiComm, debugMode, transforms, ios);
    }
}

} // end namespace adios
