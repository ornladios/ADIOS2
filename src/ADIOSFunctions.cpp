/*
 * ADIOSFunctions.cpp
 *
 *  Created on: Oct 10, 2016
 *      Author: wfg
 */

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

#include "ADIOSFunctions.h"
#include "SSupport.h"


namespace adios
{


void DumpFileToStream( const std::string fileName, std::string& fileContent )
{
    std::ifstream fileStream( fileName );

    if( fileStream.good() == false ) //check file
        throw std::ios_base::failure( "ERROR: file " + fileName +
                                      " could not be opened. Check permissions or file existence\n" );

    std::ostringstream fileSS;
    fileSS << fileStream.rdbuf();
    fileStream.close();

    fileContent = fileSS.str(); //convert to string and check
    if( fileContent.empty() ) throw std::invalid_argument( "ERROR: file " + fileName + " is empty\n" );
}


void GetQuotedValue( const char quote, const std::string::size_type& quotePosition,
                     std::string& currentTag, std::string& value )
{
    currentTag = currentTag.substr( quotePosition + 1 );
    auto nextQuotePosition = currentTag.find( quote );
    if( nextQuotePosition == currentTag.npos )
    {
        throw std::invalid_argument( "ERROR: Invalid attribute in..." + currentTag + "...check XML file\n");
    }
    value = currentTag.substr( 0, nextQuotePosition );
    currentTag = currentTag.substr( nextQuotePosition+1 );
}


void GetPairs( const std::string tag, std::vector< std::pair<const std::string, const std::string> >& pairs ) noexcept
{
    std::string currentTag( tag.substr( tag.find_first_of(" \t\n") ) ); //initialize current tag

    while( currentTag.find('=') != currentTag.npos ) //equalPosition
    {
        currentTag = currentTag.substr( currentTag.find_first_not_of(" \t\n") );
        auto equalPosition = currentTag.find('=');
        const std::string field( currentTag.substr( 0, equalPosition) );  //get field
        std::string value;

        const char quote = currentTag[equalPosition+1];
        if( quote == '\'' || quote == '"') //single quotes
        {
            //quote position?
            GetQuotedValue( quote, equalPosition+1, currentTag, value );
        }

        pairs.push_back( std::pair<const std::string, const std::string>( field, value ) );
    }
}


void GetPairsFromTag( const std::string& fileContent, const std::string tag,
                      std::vector< std::pair<const std::string, const std::string> >& pairs )
{
    if( tag.back() == '/' ) //last char is / --> "XML empty tag"
    {
        GetPairs( tag, pairs );
    }
    else if( tag[0] == '/' ) // first char is / ---> closing tag
    { }
    else // opening tag
    {
        //check for closing tagName
        const std::string tagName = tag.substr( 0, tag.find_first_of(" \t\n\r") );
        const std::string closingTagName( "</" + tagName + ">" );

        if( fileContent.find( closingTagName ) == fileContent.npos )
            throw std::invalid_argument( "ERROR: closing tag " + closingTagName + " missing, check XML file\n");

        GetPairs( tag, pairs );
    }
}


void SetMemberFromTag( const std::string tagName, const std::vector< std::pair<const std::string, const std::string> >& pairs,
                       std::string& currentGroup, std::string& hostLanguage, std::map< std::string, SGroup >& groups )
{
    if( tagName == "adios-config" ) //get Host Language
    {
        if( pairs[0].first == "host-language" )
        {
            if( HostLanguages.count( pairs[0].second ) == 0 )
                throw std::invalid_argument( "ERROR: language in adios-config not supported " + pairs[0].second + "\n" );
            else
                hostLanguage = pairs[0].second;
        }
    }
    else if( tagName == "adios-group" ) //create a new Group
    {
        bool isGroupNameFound = false;
        for( auto& pair : pairs ) //loop through all pairs
        {
            if( pair.first == "name" )
            {
                groups[ pairs[0].second ] = SGroup();
                currentGroup = pairs[0].second;
                isGroupNameFound = true;
            }

            if( isGroupNameFound == false ) throw std::invalid_argument( "ERROR: group name " + pairs[0].second + " is outside of Group scope\n" );
        }
    }
    else if( tagName == "var" ) //assign a Group variable
    {
        auto itGroup = groups.find( currentGroup );
        if( itGroup == groups.end() ) throw std::invalid_argument( "ERROR: variable " + pairs[0].second + " is outside of Group scope\n" );

        std::string name, type;
        std::string dimensionsCSV("1");

        for( auto& pair : pairs ) //loop through all pairs
        {
            if( pair.first == "name"       ) name = pair.second;
            else if( pair.first == "type"       ) type = pair.second;
            else if( pair.first == "dimensions" ) dimensionsCSV = pair.second;
        }

        auto itVariable = itGroup->second.Variables.find( name );
        if( itVariable == itGroup->second.Variables.end() )
        {
            if( type == "integer") //use copy constructor as it's a small struct
                itGroup->second.Variables[name] = std::make_shared< CVariableTemplate<int> >( dimensionsCSV );
            else if( type == "unsigned integer")
                itGroup->second.Variables[name] = std::make_shared< CVariableTemplate<unsigned int> >( dimensionsCSV );
            else if( type == "real")
                itGroup->second.Variables[name] = std::make_shared< CVariableTemplate<float> >( dimensionsCSV );
            else if( type == "double")
                itGroup->second.Variables[name] = std::make_shared< CVariableTemplate<double> >( dimensionsCSV );
        }
        else
        {
            throw std::out_of_range( "ERROR: var " + name + " is defined twice in Group " + itGroup->first + "\n" );
        }
    }
    else if( tagName == "attribute" )
    {
        auto itGroup = groups.find( currentGroup );
        if( itGroup == groups.end() ) throw std::invalid_argument( "ERROR: attribute " + pairs[0].second + " is outside of Group scope\n" );

        std::string name, path, value, type;
        for( auto& pair : pairs ) //loop through all pairs
        {
            if( pair.first == "name"       ) name = pair.second;
            else if( pair.first == "path"  )  path = pair.second;
            else if( pair.first == "value" ) value = pair.second;
            else if( pair.first == "type"  ) type = pair.second;
        }
        itGroup->second.Attributes.push_back( SAttribute( true, name, path, value, type ) ); //use copy constructor as it's a small struct
    }
    else if( tagName == "" )
    {

    }
}


void SetMembersFromXMLConfigFile( const std::string& fileContent, std::string& hostLanguage, std::map< std::string, SGroup >& groups )
{
    std::string::size_type currentPosition = fileContent.find( '<' );
    std::string currentGroup; // stores current Group name to populate m_Groups map key

    //loop through fileContent string contents
    while( currentPosition < fileContent.size() )
    {
        auto begin = fileContent.find( '<', currentPosition );
        if( begin == fileContent.npos ) break;

        //skip comment sections
        if( fileContent.find( "<!--", currentPosition ) == begin ) //found comment opening
        {
            auto endComment = fileContent.find( "-->", currentPosition );
            if( endComment == fileContent.npos ) throw std::invalid_argument( "ERROR: Open comment section, check XML file\n");
            currentPosition = endComment + 3;
            continue;
        }

        auto end = fileContent.find( '>', currentPosition );
        if( end == fileContent.npos ) break;

        //get and check current tag
        const std::string tag( fileContent.substr( begin+1, end-begin-1 ) ); //without < >
        if( tag.empty() || tag[0] == ' ' ) throw std::invalid_argument( "ERROR: Empty tag, check XML file\n");

        //skip header
        if( tag.find("?xml") == 0 )
        {
            currentPosition = end + 1;
            continue;
        }

        //get pairs of tag fields and values
        std::vector< std::pair<const std::string, const std::string> > pairs;
        GetPairsFromTag( fileContent, tag, pairs ); // from ADIOSFunctions.h

        const std::string tagName = tag.substr( 0, tag.find_first_of(" \t\n\r") );
        SetMemberFromTag( tagName, pairs, currentGroup, hostLanguage, groups );

        currentPosition = end + 1;
    }
}



void SetMembers( const std::string& fileContent, std::string& hostLanguage, std::map< std::string, SGroup >& groups )
{
    //adios-config
    std::string::size_type start = fileContent.find("<adios-config ");
    std::string::size_type length = fileContent.find("</adios-config>")-start+15;
    std::string currentContent( fileContent.substr( start, length ) );

    //remove comment sections
    std::string::size_type startComment ( currentContent.find( "<!--" ) );

    while( startComment != currentContent.npos )
    {
        std::string::size_type endComment( currentContent.find( "-->") );
        currentContent.erase( startComment, endComment-startComment+3 );
        startComment = currentContent.find( "<!--" );
    }

    //adios-config tag
    start = currentContent.find('<');
    length = currentContent.find('>')-start;
    std::string tag( currentContent.substr( start+1, length-1  )  ); //eliminate < >
    std::vector< std::pair<const std::string, const std::string> > pairs;
    GetPairsFromTag( currentContent, tag, pairs );

    for( auto& pair : pairs )
    {
        if( pair.first == "host-language" ) hostLanguage = pair.second;
    }

    //look for adios transports
    std::map< std::string, std::map<std::string,std::string> > groupsTransport;
    std::string::size_type startTransport = currentContent.find("<transport ");

    while( startTransport != currentContent.npos )
    {
        std::string::size_type endTransport( currentContent.find( "</transport>") );
        std::string::size_type endTag( currentContent.find('>', startTransport ) );

        const std::string transportTag( currentContent.substr( startTransport+1, endTag-startTransport-1 ) );
        std::cout << "Transport tag..." << transportTag << "...\n";

        const std::string transportContents( currentContent.substr( startTransport, endTransport-startTransport)  );
        currentContent.erase( startTransport, endTransport-startComment );
        startTransport = currentContent.find( "<transport " );


        std::cout << "Transport contents..." << transportContents << "...\n";
        std::cout << "Current content..." << currentContent << "...\n";
    }
}





} //end namespace
