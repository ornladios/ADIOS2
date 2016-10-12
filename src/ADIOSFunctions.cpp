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

    //Tag <adios-config
    std::string configTag;
    currentPosition = 0;
    GetSubString( "<", ">", currentContent, configTag, currentPosition );
    configTag = configTag.substr( 1, configTag.size() - 2 ); //eliminate < >
    std::vector< std::pair<const std::string, const std::string> > pairs;
    GetPairsFromTag( currentContent, configTag, pairs );

    for( auto& pair : pairs )
    {
        if( pair.first == "host-language" ) hostLanguage = pair.second;
    }

    //adios-group
    std::string xmlGroup( "Used for groups" );
    while( currentPosition != std::string::npos )
    {
        GetSubString("<adios-group ", "</adios-group>", currentContent, xmlGroup, currentPosition );
        if( xmlGroup.empty() ) break;

        std::string groupName;
        CGroup group( xmlGroup, groupName );
        groups[ groupName ] = group; //copy as it's a small object
    }




    //transports tag
//    std::map< std::string, std::map<std::string,std::string> > groupsTransport;
//    std::string::size_type startTransport = currentContent.find("<transport ");
//
//    while( startTransport != currentContent.npos )
//    {
//        std::string::size_type endTransport( currentContent.find( "</transport>") );
//        std::string::size_type endTag( currentContent.find('>', startTransport ) );
//
//        const std::string transportTag( currentContent.substr( startTransport+1, endTag-startTransport-1 ) );
//        std::vector< std::pair<const std::string, const std::string> > transportPairs;
//        std::cout << "Transport tag..." << transportTag << "...\n";
//        GetPairsFromTag( currentContent, transportTag, transportPairs );
//
//        for( auto& pair : pairs )
//        {
//            if( pair.first == "group" )
//            {
//
//            }
//        }
//        const std::string transportContents( currentContent.substr( startTransport, endTransport-startTransport)  );
//        std::cout << "Transport contents..." << transportContents << "...\n";
//
//        currentContent.erase( startTransport, endTransport-startComment + 12 );
//        std::cout << "Current content..." << currentContent << "...\n";
//
//        startTransport = currentContent.find( "<transport " );
//    }
}





} //end namespace
