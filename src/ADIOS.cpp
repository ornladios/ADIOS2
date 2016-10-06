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
    auto currentPosition = xmlFileContent.find( '<' );

    std::string currentGroup; // stores current Group name to populate m_Groups map key

    while( currentPosition < xmlFileContent.size() )
    {
        auto begin = xmlFileContent.find( '<', currentPosition );
        auto end = xmlFileContent.find( '>', currentPosition );
        if( begin == std::string::npos || end == std::string::npos )
        {
            break;
        }

        const std::string xmlItem ( xmlFileContent.substr( begin, end-begin+1 ) );
        if( xmlItem.find("<!") != std::string::npos ) //check for comments
        {
            std::cout << "Comment: ..." << xmlItem << "...\n";
            currentPosition = end + 1;
            continue;
        }

        currentPosition = end + 1;
        std::cout << "Item: ..." << xmlItem << "...\n";
    }
}




} //end namespace



