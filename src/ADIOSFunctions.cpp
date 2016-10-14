/*
 * ADIOSFunctions.cpp
 *
 *  Created on: Oct 10, 2016
 *      Author: wfg
 */

///cond
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
///endcond

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


void GetSubString ( const std::string initialTag, const std::string finalTag, const std::string content, std::string& subString,
                    std::string::size_type& currentPosition )
{
    std::string::size_type start( content.find(initialTag, currentPosition ) );
    if( start == content.npos )
    {
        subString.clear();
        currentPosition = std::string::npos;
        return;
    }
    currentPosition = start;

    std::string::size_type end( content.find(finalTag, currentPosition ) );
    if( end == content.npos )
    {
        subString.clear();
        currentPosition = std::string::npos;
        return;
    }

    subString = content.substr( start, end-start+finalTag.size() );
    currentPosition = end;
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
        const std::string tagName( tag.substr( 0, tag.find_first_of(" \t\n\r") ) );
        const std::string closingTagName( "</" + tagName + ">" );

        if( fileContent.find( closingTagName ) == fileContent.npos )
            throw std::invalid_argument( "ERROR: closing tag " + closingTagName + " missing, check XML file\n");

        GetPairs( tag, pairs );
    }
}

void SetMembers( const std::string& fileContent, std::string& hostLanguage, std::map< std::string, CGroup >& groups )
{
    //adios-config
    std::string currentContent;
    std::string::size_type currentPosition( 0 );
    GetSubString( "<adios-config ", "</adios-config>", fileContent, currentContent, currentPosition );

    //remove comment sections
    std::string::size_type startComment ( currentContent.find( "<!--" ) );

    while( startComment != currentContent.npos )
    {
        std::string::size_type endComment( currentContent.find( "-->") );
        currentContent.erase( startComment, endComment-startComment+3 );
        startComment = currentContent.find( "<!--" );
    }

    std::string tag; //use for < > tags
    std::vector< std::pair<const std::string, const std::string> > pairs; // pairs in tag

    //Tag <adios-config
    currentPosition = 0;
    GetSubString( "<", ">", currentContent, tag, currentPosition );
    tag = tag.substr( 1, tag.size() - 2 ); //eliminate < >
    GetPairsFromTag( currentContent, tag, pairs );

    for( auto& pair : pairs )
    {
        if( pair.first == "host-language" ) hostLanguage = pair.second;
    }

    //adios-group
    std::string xmlGroup;
    while( currentPosition != std::string::npos )
    {
        GetSubString("<adios-group ", "</adios-group>", currentContent, xmlGroup, currentPosition );
        if( xmlGroup.empty() ) break;

        std::string groupName;
        CGroup group( xmlGroup, groupName );
        groups[ groupName ] = group; //copy as it's a small object

        currentContent.erase( currentContent.find( xmlGroup ), xmlGroup.size() );
        currentPosition = 0;
    }

    //transport
    currentPosition = 0;
    while( currentPosition != std::string::npos )
    {
        GetSubString( "<transport ", ">", currentContent, tag, currentPosition );
        if( tag.empty() ) break;
        tag = tag.substr( 1, tag.size() - 2 ); //eliminate < >
        pairs.clear();
        GetPairsFromTag( currentContent, tag, pairs );

        std::string groupName, method, priorityStr, iterationStr;
        for( auto& pair : pairs )
        {
            if( pair.first == "group" )  groupName = pair.second;
            else if( pair.first == "method" ) method = pair.second;
            else if( pair.first == "priority" ) priorityStr = pair.second;
            else if( pair.first == "iteration" ) iterationStr = pair.second;
        }

        auto itGroup = groups.find( groupName );
        if( itGroup == groups.end() ) //not found
        {
            continue;
        }

        //lambda function to check priority and iteration
        auto lf_UIntCheck = []( const std::string method, const std::string fieldStr, const std::string fieldName, int& field )
        {
            field = 0;
            if( fieldStr.empty() == false )
            {
                field = std::stoi( fieldStr ); //throws invalid_argument
                if( field < 0 ) throw std::invalid_argument("ERROR: " + fieldName + " in transport " + method + " can't be negative\n" );
            }
        };

        int priority, iteration;
        lf_UIntCheck( method, priorityStr, "priority", priority );
        lf_UIntCheck( method, iterationStr, "iteration", iteration );

        itGroup->second.SetTransport( method, (unsigned int)priority, (unsigned int)iteration );
    }
}

#ifdef HAVE_MPI
void SetMembers( const std::string& fileContent, std::string& hostLanguage, std::map< std::string, CGroup >& groups,
                 const MPI_Comm mpiComm )
{
    //adios-config
    std::string currentContent;
    std::string::size_type currentPosition( 0 );
    GetSubString( "<adios-config ", "</adios-config>", fileContent, currentContent, currentPosition );

    //remove comment sections
    std::string::size_type startComment ( currentContent.find( "<!--" ) );

    while( startComment != currentContent.npos )
    {
        std::string::size_type endComment( currentContent.find( "-->") );
        currentContent.erase( startComment, endComment-startComment+3 );
        startComment = currentContent.find( "<!--" );
    }

    std::string tag; //use for < > tags
    std::vector< std::pair<const std::string, const std::string> > pairs; // pairs in tag

    //Tag <adios-config
    currentPosition = 0;
    GetSubString( "<", ">", currentContent, tag, currentPosition );
    tag = tag.substr( 1, tag.size() - 2 ); //eliminate < >
    GetPairsFromTag( currentContent, tag, pairs );

    for( auto& pair : pairs )
    {
        if( pair.first == "host-language" ) hostLanguage = pair.second;
    }

    //adios-group
    std::string xmlGroup;
    while( currentPosition != std::string::npos )
    {
        GetSubString("<adios-group ", "</adios-group>", currentContent, xmlGroup, currentPosition );
        if( xmlGroup.empty() ) break;

        std::string groupName;
        CGroup group( xmlGroup, groupName );
        groups[ groupName ] = group; //copy as it's a small object

        currentContent.erase( currentContent.find( xmlGroup ), xmlGroup.size() );
        currentPosition = 0;
    }

    //transport
    currentPosition = 0;
    while( currentPosition != std::string::npos )
    {
        GetSubString( "<transport ", ">", currentContent, tag, currentPosition );
        if( tag.empty() ) break;
        tag = tag.substr( 1, tag.size() - 2 ); //eliminate < >
        pairs.clear();
        GetPairsFromTag( currentContent, tag, pairs );

        std::string groupName, method, priorityStr, iterationStr;
        for( auto& pair : pairs )
        {
            if( pair.first == "group" )  groupName = pair.second;
            else if( pair.first == "method" ) method = pair.second;
            else if( pair.first == "priority" ) priorityStr = pair.second;
            else if( pair.first == "iteration" ) iterationStr = pair.second;
        }

        auto itGroup = groups.find( groupName );
        if( itGroup == groups.end() ) //not found
        {
            continue;
        }

        //lambda function to check priority and iteration
        auto lf_UIntCheck = []( const std::string method, const std::string fieldStr, const std::string fieldName, int& field )
        {
            field = 0;
            if( fieldStr.empty() == false )
            {
                field = std::stoi( fieldStr ); //throws invalid_argument
                if( field < 0 ) throw std::invalid_argument("ERROR: " + fieldName + " in transport " + method + " can't be negative\n" );
            }
        };

        int priority, iteration;
        lf_UIntCheck( method, priorityStr, "priority", priority );
        lf_UIntCheck( method, iterationStr, "iteration", iteration );

        itGroup->second.SetTransport( method, (unsigned int)priority, (unsigned int)iteration, mpiComm );
    }
}
#endif



} //end namespace
