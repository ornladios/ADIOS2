/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosXML.h basic XML parsing functionality for ADIOS config file schema
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSXML_H_
#define ADIOS2_HELPER_ADIOSXML_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <memory> //std::shared_ptr
#include <string>
#include <utility> //std::pair
#include <vector>
/// \endcond

#include "adios2/core/IO.h"
#include "adios2/core/Transform.h"

namespace adios
{

/**
 * Extracts a substring between two tags from content
 * @param initialTag
 * @param finalTag
 * @param content full string
 * @param subString if found return substring between initialTag and finalTag,
 * otherwise returns empty
 * @param currentPosition to start the search, moved forward to finalTag
 * position
 */
std::string GetSubString(const std::string initialTag,
                         const std::string finalTag, const std::string &content,
                         std::string::size_type &currentPosition);

/**
 * Determine tag type ( opening, empty, closing ), populates and returns Params
 * (map<string,string>) object with xml tag attributes
 * @param fileContent to check for missing tag closing
 * @param tag single string: key0="value0" ... keyN="valueN"
 * @return std::map of attributes: { { "key0", "value0" } ,..., { "keyN",
 * "valueN" } }
 */
Params GetTagAttributes(const std::string &fileContent, const std::string &tag);

/**
 * Set members m_Groups and m_HostLanguage from XML file content, called within
 * Init functions
 * @param fileContent file Content in a single string
 * @param mpiComm MPI Communicator passed from application passed to Transport
 * method if required
 * @param hostLanguage return the host language from fileContent
 * @param transforms return the modified transforms vector if there are
 * variables with transformations
 * @param groups passed returns the map of groups defined in fileContent
 */

void RemoveXMLComments(std::string &currentContent) noexcept;

/**
 * Called inside the ADIOS XML constructors to get contents from file,
 * broadcast and fill transforms and ios
 * @param configXMLFile
 * @param mpiComm
 * @param debugMode
 * @param transforms
 * @param ios
 */
void InitXML(const std::string configXML, const MPI_Comm mpiComm,
             const bool debugMode,
             std::vector<std::shared_ptr<Transform>> &transforms,
             std::map<std::string, IO> &ios);

void InitIOXML(const std::string &ioTag, const MPI_Comm mpiComm,
               const bool debugMode,
               std::vector<std::shared_ptr<Transform>> &transforms,
               std::map<std::string, IO> &ios);

void InitEngineXML(const std::string &engineTag, const bool debugMode, IO &io);

void InitTransportXML(const std::string &transportTag, const bool debugMode,
                      IO &io);

Params ParseParamsXML(const std::string &tag,
                      const std::string::size_type elementsStartPosition,
                      const bool debugMode);
}

#endif /* ADIOS2_HELPER_ADIOSXML_H_ */
