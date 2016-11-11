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
    m_MPIComm{ mpiComm },
    m_XMLConfigFile{ xmlConfigFile },
	m_DebugMode{ debugMode }
{
    InitXML( m_XMLConfigFile, m_MPIComm, m_DebugMode, m_HostLanguage, m_Groups );
}


ADIOS::~ADIOS( )
{ }


void ADIOS::CreateGroup( const std::string groupName, const std::string transport )
{
    if( m_DebugMode == true )
    {
        if( m_Groups.find( groupName ) != m_Groups.end() )
            throw std::invalid_argument( "ERROR: group " + groupName + " already exist, from call to CreateGroup\n" );
    }

    m_Groups.emplace( groupName, CGroup( m_HostLanguage, m_DebugMode ) );
}



void ADIOS::Open( const std::string groupName, const std::string fileName, const std::string accessMode )
{
    auto itGroup = m_Groups.find( groupName );

    if( m_DebugMode == true )
        CheckGroup( itGroup, groupName, " from call to Open with file " + fileName );

    itGroup->second.Open( fileName, accessMode );
}


void ADIOS::Close( const std::string groupName )
{
    auto itGroup = m_Groups.find( groupName );
    if( m_DebugMode == true )
    {
        CheckGroup( itGroup, groupName, " from call to Close \n" );

        if( itGroup->second.m_IsOpen == false )
            throw std::invalid_argument( "ERROR: group " + groupName + " is not open in Write function.\n" );
    }

    m_Capsule->CloseGroupBuffer( itGroup->second );
    itGroup->second.Close( );
}


void ADIOS::CreateVariable( const std::string groupName, const std::string variableName, const std::string type,
                            const std::string dimensionsCSV, const std::string transform,
                            const std::string globalDimensionsCSV, const std::string globalOffsetsCSV )
{
    auto itGroup = m_Groups.find( groupName );
    if( m_DebugMode == true )
    {
        CheckGroup( itGroup, groupName, " from call to CreateVariable \n" );
    }

    itGroup->second.CreateVariable( variableName, type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );



}

void ADIOS::MonitorGroups( std::ostream& logStream ) const
{
    for( auto& groupPair : m_Groups )
    {
        logStream << "Group:..." << groupPair.first << "\n";
        groupPair.second.Monitor( logStream );
    }
}


void ADIOS::CheckGroup( std::map< std::string, CGroup >::const_iterator itGroup, const std::string groupName, const std::string hint )
{
    if( itGroup == m_Groups.end() )
        throw std::invalid_argument( "ERROR: group " + groupName + " not found " + hint + "\n" );
}


} //end namespace
