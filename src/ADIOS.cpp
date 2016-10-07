/*
 * ADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy
 *
 */

#include <iostream>
#include <fstream>
#include <sstream>

#include "ADIOS.h"


namespace adios
{

//here assign default values of non-primitives
ADIOS::ADIOS( )
{ }


ADIOS::ADIOS( const std::string xmlConfigFile ):
    m_XMLConfigFile{ xmlConfigFile }
{ }


#ifdef HAVE_MPI
ADIOS::ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm  ):
    m_XMLConfigFile{ xmlConfigFile },
    m_IsUsingMPI{ true },
	m_MPIComm{ mpiComm }
{ }
#endif

ADIOS::~ADIOS( )
{ }


void ADIOS::Init( )
{
    std::cout << "Just testing the Init Function\n";

    if( m_IsUsingMPI == false && m_XMLConfigFile.empty() == false )
    {
        InitNoMPI( );
    }
    else
    {
        #ifdef HAVE_MPI
        InitMPI( );
        #endif
    }
}


void ADIOS::InitNoMPI( )
{
    std::string xmlFileContent;
    DumpXMLConfigFile( xmlFileContent );
    SetGroupsFromXML( xmlFileContent );

}

#ifdef MPI_VERSION
void ADIOS::InitMPI( )
{
    std::cout << "Just testing the InitMPI Function\n";
}
#endif


void ADIOS::Open( const std::string groupName, const std::string fileName, const std::string accessMode )
{
    //retrieve a group name from m_Groups
    std::cout << "Just testing the Open function\n";
}


template<class T> void ADIOS::Write( const std::string groupName, const std::string variableName, const T* values )
{
    std::cout << "Just testing the Write function\n";



}



void ADIOS::DumpXMLConfigFile( std::string& xmlFileContent ) const
{
    std::cout << "Reading XML Config File " << m_XMLConfigFile << "\n";
    std::ifstream xmlConfigStream( m_XMLConfigFile );

    if( xmlConfigStream.good() == false ) //check file
    {
        xmlConfigStream.close();
        const std::string errorMessage( "ERROR: XML Config file " + m_XMLConfigFile +
                                        " could not be opened. "
                                        "Check permissions or file existence\n");
        throw std::ios_base::failure( errorMessage );
    }

    std::ostringstream xmlStream;
    xmlStream << xmlConfigStream.rdbuf();
    xmlConfigStream.close();

    xmlFileContent = xmlStream.str(); //convert to string

    if( xmlFileContent.empty() )
    {
        throw std::invalid_argument( "ERROR: XML Config File " + m_XMLConfigFile + " is empty\n" );
    }
}
//<?xml version="1.0"?>
//<adios-config host-language="Fortran">
//  <adios-group name="writer2D">
//
//    <var name="nproc" path="/info" type="integer"/>
//    <attribute name="description" path="/info/nproc" value="Number of writers"/>
//    <var name="npx"   path="/info" type="integer"/>
//    <attribute name="description" path="/info/npx" value="Number of processors in x dimension"/>
//    <var name="npy"   path="/info" type="integer"/>
//    <attribute name="description" path="/info/npy" value="Number of processors in y dimension"/>

void ADIOS::SetGroupsFromXML( const std::string xmlFileContent )
{
    //Start with lambda functions
    auto lfGetValue = []( const char quote, const std::string::size_type& quotePosition,
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
    };

    //Get attributes field1="value1" field2="value2" for a single XML tag and
    //puts it in pairs containers first=field
    auto lfGetPairs = [&]( const std::string tag, std::vector< std::pair<const std::string, const std::string> >& pairs )
    {
        std::string currentTag( tag.substr( tag.find_first_of(" \t\n") ) ); //initialize current tag
        //currentTag = currentTag.substr( currentTag.find_first_not_of(" \t\n") ); //first field

        while( currentTag.find('=') != currentTag.npos ) //equalPosition
        {
            currentTag = currentTag.substr( currentTag.find_first_not_of(" \t\n") );
            auto equalPosition = currentTag.find('=');
            const std::string field( currentTag.substr( 0, equalPosition) );  //get field
            std::string value;

            //if( currentTag.size() < equalPosition+1 ) throw std::invalid_argument( "ERROR: Invalid tag..." + tag + "...check XML file for =\" syntax\n");

            const char quote = currentTag[equalPosition+1];
            if( quote == '\'' || quote == '"') //single quotes
            {
                //quote position?
                lfGetValue( quote, equalPosition+1, currentTag, value );
            }

            pairs.push_back( std::pair<const std::string, const std::string>( field, value ) );
        }
    };

    auto lfGetPairsFromTag = [&]( const std::string tag, std::vector< std::pair<const std::string, const std::string> >& pairs )
    {
        if( tag.back() == '/' ) //last char is / --> "XML empty tag"
        {
            lfGetPairs( tag, pairs );
        }
        else if( tag[0] == '/' ) // closing tag
        { }
        else // opening tag
        {
            //check for closing tagName
            const std::string tagName = tag.substr( 0, tag.find_first_of(" \t\n\r") );
            const std::string closingTagName( "</" + tagName + ">" );

            if( xmlFileContent.find( closingTagName ) == xmlFileContent.npos )
                throw std::invalid_argument( "ERROR: closing tag " + closingTagName + " missing, check XML file\n");

            lfGetPairs( tag, pairs );
        }
    };

    // ****************************************************************************** ////
    // Body of function STARTS ****************************************************** ////
    // ****************************************************************************** ////
    std::string::size_type currentPosition = xmlFileContent.find( '<' );

    std::string currentGroup; // stores current Group name to populate m_Groups map key
    std::string currentNonEmptyTag;

    while( currentPosition < xmlFileContent.size() )
    {
        auto begin = xmlFileContent.find( '<', currentPosition );
        if( begin == xmlFileContent.npos ) break;

        //skip comment sections
        if( xmlFileContent.find( "<!--", currentPosition ) == begin ) //found comment opening
        {
            auto endComment = xmlFileContent.find( "-->", currentPosition );
            if( endComment == xmlFileContent.npos ) throw std::invalid_argument( "ERROR: Open comment section, check XML file\n");
            currentPosition = endComment + 3;
            continue;
        }

        //get and check current tag
        auto end = xmlFileContent.find( '>', currentPosition );
        if( end == xmlFileContent.npos ) break;
        const std::string tag( xmlFileContent.substr( begin+1, end-begin-1 ) ); //without < >
        if( tag.empty() || tag[0] == ' ' ) throw std::invalid_argument( "ERROR: Empty tag, check XML file\n");
        //skip header
        if( tag.find("?xml") == 0 )
        {
            currentPosition = end + 1;
            continue;
        }

        //get pairs of tag fields and values
        std::vector< std::pair<const std::string, const std::string> > pairs;
        lfGetPairsFromTag( tag, pairs );

        std::cout << tag << "\n";

        for( auto& pair : pairs )
        {
            std::cout << "field..." << pair.first << "...value..." << pair.second << "\n";
        }
        std::cout << "\n";

        currentPosition = end + 1;
    }
}




} //end namespace



