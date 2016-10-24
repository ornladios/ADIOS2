/*
 * ADIOSFunctions.cpp
 *
 *  Created on: Oct 10, 2016
 *      Author: wfg
 */

/// \cond EXCLUDED_FROM_DOXYGEN
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
/// \endcond

#include "functions/ADIOSFunctions.h"


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

    if( fileContent.empty()  )
        throw std::invalid_argument( "ERROR: file " + fileName + " is empty\n" );
}


void GetSubString( const std::string initialTag, const std::string finalTag, const std::string content,
                   std::string& subString, std::string::size_type& currentPosition )
{
    auto lf_Wipe =[]( std::string& subString, std::string::size_type& currentPosition )
    {
        subString.clear();
        currentPosition = std::string::npos;
    };

    std::string::size_type start( content.find(initialTag, currentPosition ) );
    if( start == content.npos )
    {
        lf_Wipe( subString, currentPosition );
        return;
    }
    currentPosition = start;

    std::string::size_type end( content.find( finalTag, currentPosition ) );
    if( end == content.npos )
    {
        lf_Wipe( subString, currentPosition );
        return;
    }

    //here make sure the finalTag is not a value surrounded by " " or ' ', if so find next
    bool isValue = true;

    while( isValue == true )
    {
        std::string::size_type quotePosition = content.find( '\'', currentPosition );
        std::string::size_type doubleQuotePosition = content.find( '\"', currentPosition );

        if( ( quotePosition == content.npos && doubleQuotePosition == content.npos ) ||
            ( quotePosition == content.npos && end < doubleQuotePosition  ) ||
            ( doubleQuotePosition == content.npos && end < quotePosition  ) ||
            ( end < quotePosition && end < doubleQuotePosition )
          ) break;

        //first case
        std::string::size_type closingPosition;
        if( quotePosition < doubleQuotePosition ) //find the closing "
        {
            currentPosition = quotePosition;
            closingPosition = content.find( '\'', currentPosition+1 );
        }
        else //find the closing '
        {
            currentPosition = doubleQuotePosition;
            closingPosition = content.find( '\"', currentPosition+1 );
        }

        if( closingPosition == content.npos ) //if can't find closing it's open until the end
        {
            lf_Wipe( subString, currentPosition );
            return;
        }
        if( closingPosition < end )
        {
            currentPosition = closingPosition+1;
            continue;
        }

        //if this point is reached it means it's a value inside " " or ' ', move to the next end
        end = content.find( finalTag, currentPosition );
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
        throw std::invalid_argument( "ERROR: Invalid attribute in..." + currentTag + "...check XML file\n");

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

void SetMembers( const std::string& fileContent, const MPI_Comm mpiComm,
                 std::string& hostLanguage, std::map< std::string, CGroup >& groups )
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
            std::cout << "WARNING: group " << groupName << " in transport line not found \n";
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


void InitXML( const std::string xmlConfigFile, const MPI_Comm mpiComm,
              std::string& hostLanguage, std::map< std::string, CGroup >& groups )
{
    int xmlFileContentSize;
    std::string xmlFileContent;

    int rank;
    MPI_Comm_rank( mpiComm, &rank );

    if( rank == 0 ) //serial part
    {
        DumpFileToStream( xmlConfigFile, xmlFileContent ); //in ADIOSFunctions.h dumps all XML Config File to xmlFileContent
        xmlFileContentSize = xmlFileContent.size( ) + 1; // add one for the null character

        MPI_Bcast( &xmlFileContentSize, 1, MPI_INT, 0, mpiComm ); //broadcast size for allocation
        MPI_Bcast( (char*)xmlFileContent.c_str(), xmlFileContentSize, MPI_CHAR, 0, mpiComm ); //broadcast contents
    }
    else
    {
        MPI_Bcast( &xmlFileContentSize, 1, MPI_INT, 0, mpiComm  ); //receive size

        char* xmlFileContentMPI = new char[ xmlFileContentSize ]; //allocate xml C-char
        MPI_Bcast( xmlFileContentMPI, xmlFileContentSize, MPI_CHAR, 0, mpiComm ); //receive xml C-char
        xmlFileContent.assign( xmlFileContentMPI ); //copy to a string

        delete []( xmlFileContentMPI ); //delete char* needed for MPI, might add size is moving to C++14 for optimization, avoid memory leak
    }

    SetMembers( xmlFileContent,  mpiComm, hostLanguage,  groups );
}



} //end namespace
