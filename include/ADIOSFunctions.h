/*
 * ADIOSFunctions.h Describe functions used by ADIOS class
 *
 *  Created on: Oct 10, 2016
 *      Author: wfg
 */

#ifndef ADIOSFUNCTIONS_H_
#define ADIOSFUNCTIONS_H_

#include <string>
#include <vector>
#include <map>

#include "SGroup.h"


namespace adios
{

/**
 * Opens and checks for file and dumps contents to a single string
 * @param fileName file to be opened
 * @param fileContents output contains the entire file
 */
void DumpFileToStream( const std::string fileName, std::string& fileContents );

/**
 * Extracts the value inside quotes in a string currentTag ( Example: currentTag --> field1="value1" field2="value2" )
 * @param quote double " or single '
 * @param quotePosition position of the opening quote in currentTag
 * @param currentTag initial tag value, modified by cutting the first found " " portion, currentTag --> field2="value2"
 * @param value value1 in the example above
 */
void GetQuotedValue( const char quote, const std::string::size_type& quotePosition,
                     std::string& currentTag, std::string& value );

/**
 * Get attributes field1="value1" field2="value2" by looping through a single XML tag
 * @param tag field0="value0" field1="value1" in a single string
 * @param pairs pairs[0].first=field0 pairs[0].second=value0 pairs[1].first=field1 pairs[1].second=value1
 */
void GetPairs( const std::string tag, std::vector< std::pair<const std::string, const std::string> >& pairs ) noexcept;


/**
 * Determine tag type and call GetPairs to populate pairs
 * @param fileContent file Content in a single string
 * @param tag field0="value0" field1="value1" in a single string
 * @param pairs pairs[0].first=field0 pairs[0].second=value0 pairs[1].first=field1 pairs[1].second=value1
 */
void GetPairsFromTag( const std::string& fileContent, const std::string tag,
                      std::vector< std::pair<const std::string, const std::string> >& pairs );


/**
 * Sets a member of ADIOS class hostLanguage or groups based based on a single tagName, called in while loop of SetMembersFromXMLConfigFile
 * @param tagName field0="value0" field1="value1" in a single string
 * @param pairs pairs[0].first=field0 pairs[0].second=value0 pairs[1].first=field1 pairs[1].second=value1
 * @param currentGroup current group to be populated
 * @param hostLanguage to be populated if tagName="adios-config"
 * @param groups to be populated based on currentGroup and tagName adios-group, var, attribute
 */
void SetMemberFromTag( const std::string tagName, const std::vector< std::pair<const std::string, const std::string> >& pairs,
                       std::string& currentGroup, std::string& hostLanguage, std::map< std::string, SGroup >& groups );


/**
 * Loops through fileContent for tags and sets group and hostLanguage members in class ADIOS calling SetMemberFromTag
 * @param fileContent XML file Content in a single string
 * @param hostLanguage to be set
 * @param groups to be set
 */
void SetMembersFromXMLConfigFile( const std::string& fileContent, std::string& hostLanguage, std::map< std::string, SGroup >& groups );







/**
 * Set members
 * @param fileContent
 * @param hostLanguage
 * @param groups
 */
void SetMembers( const std::string& fileContent, std::string& hostLanguage, std::map< std::string, SGroup >& groups );





}



#endif /* ADIOSFUNCTIONS_H_ */
