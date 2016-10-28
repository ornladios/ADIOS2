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


ADIOS::ADIOS( const std::string xmlConfigFile, const bool debugMode ):
    m_XMLConfigFile{ xmlConfigFile },
    m_DebugMode{ debugMode }
{
    InitXML( m_XMLConfigFile, m_MPIComm, m_DebugMode, m_HostLanguage, m_Groups );
}


ADIOS::ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm, const bool debugMode  ):
    m_XMLConfigFile{ xmlConfigFile },
	m_MPIComm{ mpiComm },
	m_DebugMode{ debugMode }
{
    InitXML( m_XMLConfigFile, m_MPIComm, m_DebugMode, m_HostLanguage, m_Groups );
}


ADIOS::~ADIOS( )
{ }


void ADIOS::Open( const std::string groupName, const std::string fileName, const std::string accessMode )
{
    if( m_DebugMode == true )
        CheckGroup( groupName, " from call to Open with file " + fileName );

    m_Groups.at( groupName ).Open( fileName, accessMode );
}


void ADIOS::Write( const std::string groupName, const std::string variableName, const void* values )
{
    if( m_DebugMode == true )
        CheckGroup( groupName, " from call to Write with variable " + variableName );

    m_Groups.at( groupName ).Write( variableName, values );
}


void ADIOS::Close( const std::string groupName )
{
    if( m_DebugMode == true )
        CheckGroup( groupName, " from call to Close." );

    m_Groups.at( groupName ).Close( ); //here must manage closing Capsule and Transport
    m_Groups.erase( groupName ); //make group unavailable
}


void ADIOS::MonitorGroups( std::ostream& logStream ) const
{
    for( auto& groupPair : m_Groups )
    {
        logStream << "Group:..." << groupPair.first << "\n";
        groupPair.second.Monitor( logStream );
    }
}


void ADIOS::CheckGroup( const std::string groupName, const std::string hint )
{
    if( m_Groups.count( groupName ) == 0 )
        throw std::invalid_argument( "ERROR: group " + groupName + " not found " + hint + "\n" );
}


} //end namespace
