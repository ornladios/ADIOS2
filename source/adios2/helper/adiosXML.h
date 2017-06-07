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
#include <string>
#include <utility> //std::pair
#include <vector>
/// \endcond

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
void GetSubString(const std::string initialTag, const std::string finalTag,
                  const std::string content, std::string &subString,
                  std::string::size_type &currentPosition);

/**
 * Extracts the value inside quotes in a string currentTag ( Example: currentTag
 * --> field1="value1" field2="value2" )
 * @param quote double " or single '
 * @param quotePosition position of the opening quote in currentTag
 * @param currentTag initial tag value, modified by cutting the first found " "
 * portion, currentTag --> field2="value2"
 * @param value value1 in the example above
 */
void GetQuotedValue(const char quote,
                    const std::string::size_type &quotePosition,
                    std::string &currentTag, std::string &value);

/**
 * Get attributes field1="value1" field2="value2" by looping through a single
 * XML tag
 * @param tag field0="value0" field1="value1" in a single string
 * @param pairs pairs[0].first=field0 pairs[0].second=value0
 * pairs[1].first=field1 pairs[1].second=value1
 */
void GetPairs(const std::string tag,
              std::vector<std::pair<const std::string, const std::string>>
                  &pairs) noexcept;

/**
 * Determine tag type and call GetPairs to populate pairs
 * @param fileContent file Content in a single string
 * @param tag field0="value0" field1="value1" in a single string
 * @param pairs pairs[0].first=field0 pairs[0].second=value0
 * pairs[1].first=field1 pairs[1].second=value1
 */
void GetPairsFromTag(
    const std::string &fileContent, const std::string tag,
    std::vector<std::pair<const std::string, const std::string>> &pairs);

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
// void SetMembers( const std::string& fileContent, const MPI_Comm mpiComm,
//                 std::string& hostLanguage, std::vector<
//                 std::shared_ptr<Transform> >& transforms,
//                 std::map< std::string, Group >& groups );

/**
 * Called inside the ADIOS XML constructors to get contents from file, broadcast
 * and set hostLanguage and groups from ADIOS class
 * @param xmlConfigFile xml config file name
 * @param mpiComm communicator used from broadcasting
 * @param debugMode from ADIOS m_DebugMode passed to CGroup in groups
 * @param hostLanguage set from host-language in xml file
 * @param transforms return the modified transforms vector if there are
 * variables with transformations
 * @param groups passed returns the map of groups defined in fileContent
 */
// void InitXML( const std::string xmlConfigFile, const MPI_Comm mpiComm, const
// bool debugMode,
//              std::string& hostLanguage, std::vector<
//              std::shared_ptr<Transform> >& transforms,
//              std::map< std::string, Group >& groups );
}

#endif /* ADIOS2_HELPER_ADIOSXML_H_ */
