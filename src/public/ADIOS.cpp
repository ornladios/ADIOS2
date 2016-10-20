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


ADIOS::ADIOS( const bool debugMode ):
    m_DebugMode{ debugMode }
{ }


ADIOS::ADIOS( const std::string xmlConfigFile, const bool debugMode ):
    m_XMLConfigFile{ xmlConfigFile },
    m_DebugMode{ debugMode }
{
    InitXML( m_XMLConfigFile, m_MPIComm, m_HostLanguage, m_Groups );
}


ADIOS::ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm, const bool debugMode  ):
    m_XMLConfigFile{ xmlConfigFile },
	m_MPIComm{ mpiComm },
	m_DebugMode{ debugMode }
{
    InitXML( m_XMLConfigFile, m_MPIComm, m_HostLanguage, m_Groups );
}


ADIOS::~ADIOS( )
{ }


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
