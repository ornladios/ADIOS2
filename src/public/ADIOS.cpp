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
    m_DebugMode{ debugMode },
    m_Capsule{ CCapsule( debugMode ) }
{
    InitXML( m_XMLConfigFile, m_MPIComm, m_DebugMode, m_HostLanguage, m_Groups );
}


ADIOS::ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm, const bool debugMode  ):
    m_MPIComm{ mpiComm },
    m_XMLConfigFile{ xmlConfigFile },
	m_DebugMode{ debugMode },
	m_Capsule{ CCapsule( mpiComm, debugMode ) }
{
    InitXML( m_XMLConfigFile, m_MPIComm, m_DebugMode, m_HostLanguage, m_Groups );
}


ADIOS::ADIOS( const MPI_Comm mpiComm, const bool debugMode ):
    m_MPIComm{ mpiComm },
    m_DebugMode{ debugMode },
    m_Capsule{ CCapsule( mpiComm, debugMode ) }
{ }


ADIOS::~ADIOS( )
{ }


void ADIOS::CreateGroup( const std::string groupName )
{
    if( m_DebugMode == true )
    {
        if( m_Groups.find( groupName ) != m_Groups.end() )
            throw std::invalid_argument( "ERROR: group " + groupName + " already exist, from call to CreateGroup\n" );
    }

    m_Groups.emplace( groupName, CGroup( m_HostLanguage, m_DebugMode ) );
}


void ADIOS::Open( const std::string groupName, const std::string streamName, const std::string accessMode,
                  unsigned long long int maxBufferSize )
{
    auto itGroup = m_Groups.find( groupName );

    if( m_DebugMode == true )
        CheckGroup( itGroup, groupName, " from call to Open with stream " + streamName +
                                        ", access mode " + accessMode );

    //Set Group
    CGroup& group = itGroup->second;
    group.m_IsOpen = true;
    group.m_StreamName = streamName;
    //Open Stream/Buffer/Transport in capsule
    m_Capsule.Open( streamName, accessMode, maxBufferSize, itGroup->second.m_Transport );
}


void ADIOS::Close( const std::string groupName ) //need to think if it's a group or stream
{
    auto itGroup = m_Groups.find( groupName );
    if( m_DebugMode == true )
    {
        CheckGroup( itGroup, groupName, " from call to Close \n" );

        if( itGroup->second.m_IsOpen == false )
            throw std::invalid_argument( "ERROR: group " + groupName + " is not open in Write function.\n" );
    }

    CGroup& group = itGroup->second;

    m_Capsule.Close( group.m_StreamName ); // first close any stream associated with group
    group.m_StreamName.clear();
    group.m_Transport.clear();
    group.m_IsOpen = false;
}


void ADIOS::CreateVariable( const std::string groupName, const std::string variableName, const std::string type,
                            const std::string dimensionsCSV, const std::string transform,
                            const std::string globalDimensionsCSV, const std::string globalOffsetsCSV )
{
    auto itGroup = m_Groups.find( groupName );
    if( m_DebugMode == true )
        CheckGroup( itGroup, groupName, " from call to CreateVariable \n" );

    itGroup->second.CreateVariable( variableName, type, dimensionsCSV, transform, globalDimensionsCSV, globalOffsetsCSV );
}


void ADIOS::CreateAttribute( const std::string groupName, const std::string attributeName,
                             const std::string type, const std::string value,
                             const std::string globalDimensionsCSV, const std::string globalOffsetsCSV )
{
    auto itGroup = m_Groups.find( groupName );
    if( m_DebugMode == true )
        CheckGroup( itGroup, groupName, " from call to CreateAttribute \n" );

    itGroup->second.CreateAttribute( attributeName, type, value, globalDimensionsCSV, globalOffsetsCSV );
}


void ADIOS::SetTransport( const std::string groupName, const std::string transport )
{
    auto itGroup = m_Groups.find( groupName );
    if( m_DebugMode == true )
        CheckGroup( itGroup, groupName, " from call to SetTransport \n" );

    itGroup->second.m_Transport = transport;
}



void ADIOS::MonitorGroups( std::ostream& logStream ) const
{
    for( auto& groupPair : m_Groups )
    {
        logStream << "Group:..." << groupPair.first << "\n";
        groupPair.second.Monitor( logStream );
    }
}

//PRIVATE FUNCTIONS BELOW
void ADIOS::CheckGroup( std::map< std::string, CGroup >::const_iterator itGroup, const std::string groupName, const std::string hint )
{
    if( itGroup == m_Groups.end() )
        throw std::invalid_argument( "ERROR: group " + groupName + " not found " + hint + "\n" );
}


} //end namespace
