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
{
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
}


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
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
    InitXML( m_XMLConfigFile, m_MPIComm, m_DebugMode, m_HostLanguage, m_Groups );
}


ADIOS::ADIOS( const MPI_Comm mpiComm, const bool debugMode ):
    m_MPIComm{ mpiComm },
    m_DebugMode{ debugMode }
{
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
}


ADIOS::~ADIOS( )
{ }


void ADIOS::SetMaxBufferSize( const std::string streamName, const size_t maxBufferSize )
{
    auto itCapsule = m_Capsules.find( streamName );
    if( m_DebugMode == true )
        CheckCapsule( itCapsule, streamName, " from call to SetMaxBufferSize\n" );

    itCapsule->second.m_MaxBufferSize = maxBufferSize;
}


void ADIOS::SetCurrentGroup( const std::string streamName, const std::string groupName )
{
    auto itCapsule = m_Capsules.find( streamName );
    if( m_DebugMode == true )
        CheckCapsule( itCapsule, streamName, " from call to SetCurrentGroup\n" );

    itCapsule->second.m_CurrentGroup = groupName;
}


void ADIOS::Close( const std::string streamName, const int transportIndex ) //close stream
{
    auto itCapsule = m_Capsules.find( streamName );
    if( m_DebugMode == true )
        CheckCapsule( itCapsule, streamName, " from call to Close\n" );

    itCapsule->second.Close( transportIndex );
}


void ADIOS::DeclareGroup( const std::string groupName )
{
    if( m_DebugMode == true )
    {
        if( m_Groups.count( groupName ) == 1 )
            throw std::invalid_argument( "ERROR: group " + groupName + " already exist, from call to CreateGroup\n" );
    }

    m_Groups.emplace( groupName, CGroup( m_HostLanguage, m_DebugMode ) );
}


void ADIOS::DefineVariable( const std::string groupName, const std::string variableName, const std::string type,
                            const std::string dimensionsCSV,
                            const std::string globalDimensionsCSV, const std::string globalOffsetsCSV )
{
    auto itGroup = m_Groups.find( groupName );
    if( m_DebugMode == true )
        CheckGroup( itGroup, groupName, " from call to CreateVariable \n" );

    itGroup->second.DefineVariable( variableName, type, dimensionsCSV, globalDimensionsCSV, globalOffsetsCSV );
}


void ADIOS::SetTransform( const std::string groupName, const std::string variableName, const std::string transform )
{
    auto itGroup = m_Groups.find( groupName );
    if( m_DebugMode == true ) //check group and transform
    {
        CheckGroup( itGroup, groupName, " from call to SetTransform \n" );

        if( SSupport::Transforms.count( transform ) == 0 )
            throw std::invalid_argument( "ERROR: transform method " + transform + " not supported, in call to SetTransform\n" );
    }

    //get method:compressionLevel from transform
    std::string method( transform );
    int compressionLevel = 0;

    auto colonPosition = transform.find( ":" );

    if( colonPosition != transform.npos )
    {
        if( m_DebugMode == true )
        {
            if( colonPosition == transform.size() - 1 )
                throw std::invalid_argument( "ERROR: wrong format for transform " + transform + ", in call to SetTransform\n" );
        }

        method = transform.substr( 0, colonPosition );
        compressionLevel = std::stoi( transform.substr( colonPosition+1 ) );
    }

    int transformIndex = -1;
    for( unsigned int i = 0; i < m_Transforms.size(); ++i )
    {
        if( m_Transforms[i]->m_Method == transform )
        {
            transformIndex = i;
            break;
        }
    }

    if( transformIndex == -1 ) //not found, then create a new transform
    {
        if( transform == "bzip2" )
        {
            m_Transforms.push_back( std::make_shared<CBZIP2>( ) );
        }
    }

    itGroup->second.SetTransform( variableName, transformIndex, compressionLevel );
}





void ADIOS::DefineAttribute( const std::string groupName, const std::string attributeName,
                             const std::string type, const std::string value )

{
    auto itGroup = m_Groups.find( groupName );
    if( m_DebugMode == true )
        CheckGroup( itGroup, groupName, " from call to CreateAttribute \n" );

    itGroup->second.DefineAttribute( attributeName, type, value );
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
void ADIOS::CheckGroup( std::map< std::string, CGroup >::const_iterator itGroup,
                        const std::string groupName, const std::string hint ) const
{
    if( itGroup == m_Groups.end() )
        throw std::invalid_argument( "ERROR: group " + groupName + " not found " + hint + "\n" );
}

void ADIOS::CheckCapsule( std::map< std::string, CCapsule >::const_iterator itCapsule,
                          const std::string streamName, const std::string hint ) const
{
    if( itCapsule == m_Groups.end() )
        throw std::invalid_argument( "ERROR: stream (or file) " + streamName + " not created with Open , " + hint + "\n" );
}


} //end namespace
