/*
 * ADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy
 *
 */

/// \cond EXCLUDE_FROM_DOXYGEN
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
/// \endcond

#include "public/ADIOS.h"
#include "functions/ADIOSFunctions.h"


namespace adios
{


ADIOS::ADIOS( )
{ }


ADIOS::ADIOS( const std::string xmlConfigFile ):
    m_XMLConfigFile{ xmlConfigFile }
{ }


ADIOS::ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm  ):
    m_XMLConfigFile{ xmlConfigFile },
	m_MPIComm{ mpiComm }
{ }


ADIOS::~ADIOS( )
{ }


void ADIOS::Init( )
{
    int xmlFileContentSize;
    std::string xmlFileContent;

    int rank;
    MPI_Comm_rank( m_MPIComm, &rank );

    if( rank == 0 ) //serial part
    {
        std::string xmlFileContent;
        DumpFileToStream( m_XMLConfigFile, xmlFileContent ); //in ADIOSFunctions.h dumps all XML Config File to xmlFileContent
        xmlFileContentSize = xmlFileContent.size( ) + 1; // add one for the null character

        MPI_Bcast( &xmlFileContentSize, 1, MPI_INT, 0, m_MPIComm  ); //broadcast size for allocation
        MPI_Bcast( (char*)xmlFileContent.c_str(), xmlFileContentSize, MPI_CHAR, 0, m_MPIComm );
        SetMembers( xmlFileContent, m_HostLanguage,  m_Groups, m_MPIComm );
    }
    else
    {
        MPI_Bcast( &xmlFileContentSize, 1, MPI_INT, 0, m_MPIComm  ); //broadcast size
        char* xmlFileContentMPI = new char[ xmlFileContentSize ]; //allocate C char
        MPI_Bcast( xmlFileContentMPI, xmlFileContentSize, MPI_CHAR, 0, m_MPIComm ); //receive from rank=0

        xmlFileContent.assign( xmlFileContentMPI );
        SetMembers( xmlFileContent, m_HostLanguage,  m_Groups, m_MPIComm );
    }
}


void ADIOS::Open( const std::string groupName, const std::string fileName, const std::string accessMode )
{
    m_Groups.at( groupName ).Open( fileName, accessMode );
}


template<class T> void ADIOS::Write( const std::string groupName, const std::string variableName, const T* values )
{
    m_Groups.at( groupName ).Write( variableName, values );
}

void ADIOS::Close( const std::string groupName )
{
    m_Groups.at( groupName ).Close();
}

void ADIOS::MonitorGroups( std::ostream& logStream ) const
{
    for( auto& groupPair : m_Groups )
    {
        logStream << "Group:..." << groupPair.first << "\n";
        groupPair.second.Monitor( logStream );
    }
}

void ADIOS::CheckGroup( const std::string groupName )
{
    auto it = m_Groups.find( groupName );
    if( it == m_Groups.end() ) throw std::invalid_argument( "ERROR: group " + groupName + " not found\n" );
}



} //end namespace
