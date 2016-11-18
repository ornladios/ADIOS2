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
#include <thread>  //std::thread
#include <cstring> //std::memcpy
/// \endcond

#include "functions/ADIOSFunctions.h"
#include "public/SSupport.h"


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
    auto lf_Wipe = []( std::string& subString, std::string::size_type& currentPosition )
    {
        subString.clear();
        currentPosition = std::string::npos;
    };

    auto lf_SetPositions = []( const char quote, const std::string::size_type quotePosition, const std::string& content,
                               std::string::size_type& currentPosition, std::string::size_type& closingQuotePosition )
    {
        currentPosition = quotePosition;
        closingQuotePosition = content.find( quote, currentPosition+1 );
    };


    //BODY OF FUNCTION STARTS HERE
    std::string::size_type start( content.find( initialTag, currentPosition ) );
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
        std::string::size_type singleQuotePosition = content.find( '\'', currentPosition );
        std::string::size_type doubleQuotePosition = content.find( '\"', currentPosition );

        if( ( singleQuotePosition == content.npos && doubleQuotePosition == content.npos ) ||
            ( singleQuotePosition == content.npos && end < doubleQuotePosition  ) ||
            ( doubleQuotePosition == content.npos && end < singleQuotePosition  ) ||
            ( end < singleQuotePosition && end < doubleQuotePosition )
          ) break;

        //find the closing corresponding quote
        std::string::size_type closingQuotePosition;

        if( singleQuotePosition == content.npos ) //no ' anywhere
            lf_SetPositions( '\"', doubleQuotePosition, content, currentPosition, closingQuotePosition );
        else if( doubleQuotePosition == content.npos ) //no " anywhere
            lf_SetPositions( '\'', singleQuotePosition, content, currentPosition, closingQuotePosition );
        else
        {
            if( singleQuotePosition < doubleQuotePosition )
                lf_SetPositions( '\'', singleQuotePosition, content, currentPosition, closingQuotePosition );
            else //find the closing "
                lf_SetPositions( '\"', doubleQuotePosition, content, currentPosition, closingQuotePosition );
        }

        if( closingQuotePosition == content.npos ) //if can't find closing it's open until the end
        {
            lf_Wipe( subString, currentPosition );
            return;
        }

        currentPosition = closingQuotePosition+1;

        if( closingQuotePosition < end )
            continue;

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
        const std::string tagName( tag.substr( 0, tag.find_first_of(" \t\n\r") ) );
        const std::string closingTagName( "</" + tagName + ">" ); //check for closing tagName

        if( fileContent.find( closingTagName ) == fileContent.npos )
            throw std::invalid_argument( "ERROR: closing tag " + closingTagName + " missing, check XML file\n");

        GetPairs( tag, pairs );
    }
}


void SetMembers( const std::string& fileContent, const MPI_Comm mpiComm, const bool debugMode,
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

    //Tag <adios-config
    currentPosition = 0;

    std::string tag; //use for < > tags
    GetSubString( "<adios-config", ">", currentContent, tag, currentPosition );
    tag = tag.substr( 1, tag.size() - 2 ); //eliminate < >

    std::vector< std::pair<const std::string, const std::string> > pairs; // pairs in tag
    GetPairsFromTag( currentContent, tag, pairs );

    for( auto& pair : pairs )
        if( pair.first == "host-language" )
            hostLanguage = pair.second;

    if( debugMode == true )
    {
        if( SSupport::HostLanguages.count( hostLanguage ) == 0 )
            throw std::invalid_argument("ERROR: host language " + hostLanguage + " not supported.\n" );

        if( hostLanguage.empty() == true )
            throw std::invalid_argument("ERROR: host language is empty.\n" );
    }

    //adios-group
    currentPosition = 0;

    while( currentPosition != std::string::npos )
    {
        std::string xmlGroup;
        GetSubString("<adios-group ", "</adios-group>", currentContent, xmlGroup, currentPosition ); //Get all group contents

        if( xmlGroup.empty() ) //no more groups to find
            break;

        //get group name
        std::string::size_type groupPosition( 0 );
        GetSubString( "<adios-group ", ">", xmlGroup, tag, groupPosition );
        if( debugMode == true )
        {
            if( tag.size() < 2 )
                throw std::invalid_argument( "ERROR: wrong tag " + tag + " in adios-group\n" ); //check < or <=
        }

        tag = tag.substr( 1, tag.size() - 2 ); //eliminate < >
        GetPairsFromTag( xmlGroup, tag, pairs );
        std::string groupName;

        for( auto& pair : pairs )
        {
            if( pair.first == "name")
                groupName = pair.second;
        }

        if( debugMode == true )
        {
            if( groupName.empty() )
                throw std::invalid_argument( "ERROR: group name not found. \n" );

            if( groups.count( groupName ) == 1 ) //group exists
                throw std::invalid_argument( "ERROR: group " + groupName + " defined twice.\n" );
        }

        groups.emplace( groupName, CGroup( hostLanguage, xmlGroup, debugMode ) );

        currentContent.erase( currentContent.find( xmlGroup ), xmlGroup.size() );
        currentPosition = 0;
    }

    //transport
    //lambda function to check priority and iteration casting to unsigned int
    auto lf_UIntCheck = []( const std::string method, const std::string fieldStr, const std::string fieldName,
                            const bool debugMode, int& field )
    {
        field = 0;
        if( fieldStr.empty() == false )
        {
            field = std::stoi( fieldStr ); //throws invalid_argument

            if( debugMode == true )
            {
                if( field < 0 )
                    throw std::invalid_argument("ERROR: " + fieldName + " in transport " + method + " can't be negative\n" );
            }
        }
    };

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
        if( debugMode == true )
        {
            if( itGroup == groups.end() ) //not found
                throw std::invalid_argument( "ERROR: in transport " + method + " group " + groupName + " not found.\n" );
        }

        int priority, iteration;
        lf_UIntCheck( method, priorityStr, "priority", debugMode, priority );
        lf_UIntCheck( method, iterationStr, "iteration", debugMode, iteration );

        itGroup->second.m_Transport = method;
        //here do something with the capsule
    }
}


void InitXML( const std::string xmlConfigFile, const MPI_Comm mpiComm, const bool debugMode,
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

    SetMembers( xmlFileContent,  mpiComm, debugMode, hostLanguage,  groups );
}


unsigned long long int GetTotalSize( const std::vector<unsigned long long int>& dimensions )
{
    unsigned long long int product = 1;

    for( const auto dimension : dimensions )
        product *= dimension;

    return product;
}


void MemcpyThreads( void* destination, const void* source, std::size_t count, const unsigned int cores )
{

    const size_t stride = (size_t) std::floor( count/cores );
    const size_t remainder =  (size_t) count % cores;
    std::vector<std::thread> memcpyThreads;
    memcpyThreads.reserve( cores );

    for( unsigned int core = 0; core < cores; ++core )
    {
        const unsigned int initial = stride * core;

        if( core == cores-1 )
            memcpyThreads.push_back( std::thread( std::memcpy( &destination[initial], &source[initial], remainder ) ) );
        else
            memcpyThreads.push_back( std::thread( std::memcpy( &destination[initial], &source[initial], stride ) ) );
    }

    //Now join the threads
    std::for_each( memcpyThreads.begin(), memcpyThreads.end(), []( std::thread& thread ){ thread.join(); }  );

}



//Write helper functions
void WriteChar( CGroup& group, const std::string variableName, const char* values, CCapsule& capsule, const unsigned int cores )
{
    if( group.m_DebugMode == true )
    {
        const std::string type( group.m_Variables.at( variableName ).first );
        if( type != "char" )
            throw std::invalid_argument( "ERROR: variable " + variableName + " is not char\n" );
    }

    const unsigned int index = group.m_Variables.at( variableName ).second;
    SVariable<char>& variable = group.m_Char[index];
    variable.m_Values = values;
    auto localDimensions = group.GetDimensions( variable.m_DimensionsCSV );

    if( variable.m_GlobalBoundsIndex > -1 ) //global variable
    {
        auto globalDimensions = group.GetDimensions( group.m_GlobalBounds[ variable.m_GlobalBoundsIndex ].first );
        auto globalOffsets = group.GetDimensions( group.m_GlobalBounds[ variable.m_GlobalBoundsIndex ].second );

        //capsule.Write( group.m_StreamName, variable.m_Values, sizeof(char), localDimensions, globalDimensions, globalOffsets );
    }
    else //write local variable
    {
        const unsigned long long int size = GetTotalSize( localDimensions );
        capsule.Write( group.m_StreamName, variable.m_Values, size, cores );
    }
}


} //end namespace
