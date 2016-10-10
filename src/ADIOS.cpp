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
#include <utility>

#include "ADIOS.h"
#include "ADIOSFunctions.h"


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
    DumpFileToStream( m_XMLConfigFile, xmlFileContent ); //in ADIOSFunctions.h
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


void ADIOS::SetGroupsFromXML( const std::string xmlFileContent )
{
    //lambda function that populates ADIOS members
    auto lf_Populate= [&]( std::string& currentGroup,
                           const std::string tagName,
                           std::vector< std::pair<const std::string, const std::string> >& pairs )
    {
        if( tagName == "adios-config" ) //get Host Language
        {
            if( pairs[0].first == "host-language" )
            {
                if( HostLanguages.count( pairs[0].second ) == 0 )
                    throw std::invalid_argument( "ERROR: language in adios-config not supported " + pairs[0].second + "\n" );
                else
                    m_HostLanguage = pairs[0].second;
            }
        }
        else if( tagName == "adios-group" ) //create a new Group
        {
            if( pairs[0].first == "name" )
            {
                m_Groups[ pairs[0].second ] = SGroup( );
                currentGroup = pairs[0].second;
            }
        }
        else if( tagName == "var" ) //assign a Group variable
        {
            auto itGroup = m_Groups.find( currentGroup );
            if( itGroup == m_Groups.end() ) throw std::invalid_argument( "ERROR: variable " + pairs[0].second + " is outside of Group scope\n" );

            std::string name, type, dimensionsCSV;

            for( auto& pair : pairs ) //lopp through all pairs
            {
                     if( pair.first == "name"       ) name = pair.second;
                else if( pair.first == "type"       ) type = pair.second;
                else if( pair.first == "dimensions" ) dimensionsCSV = pair.second;
            }

            auto Variables = itGroup->second.Variables;
            auto itVariable = Variables.find( name );
            if( itVariable == Variables.end() )
            {
                //might move to make_unique for C++14
                if( type == "integer")
                    Variables[name] = std::unique_ptr<CVariable>( new CVariableTemplate<int>( dimensionsCSV ) );
                if( type == "unsigned integer")
                    Variables[name] = std::unique_ptr<CVariable>( new CVariableTemplate<unsigned int>( dimensionsCSV ) );
                else if( type == "real")
                    Variables[name] = std::unique_ptr<CVariable>( new CVariableTemplate<float>( dimensionsCSV ) );
                else if( type == "double")
                    Variables[name] = std::unique_ptr<CVariable>( new CVariableTemplate<double>( dimensionsCSV ) );
            }
            else
            {
                throw std::out_of_range( "ERROR: var " + name + " is defined twice in Group " + itGroup->first + "\n" );
            }
        }
        else if( tagName == "attribute" )
        {
            std::cout << "Found an attribute\n";
        }
    }; //end lambda function

    std::string::size_type currentPosition = xmlFileContent.find( '<' );

    std::string currentGroup; // stores current Group name to populate m_Groups map key
    std::string currentNonEmptyTag;

    //loop through xmlFileContent string contents
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

        auto end = xmlFileContent.find( '>', currentPosition );
        if( end == xmlFileContent.npos ) break;

        //get and check current tag
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
        GetPairsFromTag( xmlFileContent, tag, pairs ); // from ADIOSFunctions.h

        const std::string tagName = tag.substr( 0, tag.find_first_of(" \t\n\r") );
        lf_Populate( currentGroup, tagName, pairs );

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



