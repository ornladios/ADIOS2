/*
 * ADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy
 */

/// \cond EXCLUDE_FROM_DOXYGEN
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
/// \endcond

#include "ADIOS.h"
#include "functions/adiosFunctions.h"


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
    InitXML( m_XMLConfigFile, m_MPIComm, m_DebugMode, m_HostLanguage, m_Transforms, m_Groups );
}


ADIOS::ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm, const bool debugMode  ):
    m_MPIComm{ mpiComm },
    m_XMLConfigFile{ xmlConfigFile },
	m_DebugMode{ debugMode }
{
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
    InitXML( m_XMLConfigFile, m_MPIComm, m_DebugMode, m_HostLanguage, m_Transforms, m_Groups );
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


void ADIOS::DeclareGroup( const std::string groupName )
{
    if( m_DebugMode == true )
    {
        if( m_Groups.count( groupName ) == 1 )
            throw std::invalid_argument( "ERROR: group " + groupName + " already exist, from call to DeclareGroup\n" );
    }

    m_Groups.emplace( groupName, Group( m_HostLanguage, m_DebugMode ) );
}


void ADIOS::SetGroup( const unsigned int handler, const std::string groupName )
{
    auto itEngine = m_Engines.find( handler );
    auto itGroup = m_Groups.find( groupName );

    if( m_DebugMode == true )
    {
        CheckEngine( itEngine, handler, " in call to SetGroup.\n" );
        CheckGroup( itGroup, groupName, " in call to SetGroup.\n" );
    }

    itEngine->second->m_Group = &( itGroup->second );
}


const unsigned int ADIOS::Open( const std::string name, const std::string accessMode,
                                MPI_Comm mpiComm, const std::string methodName )
{
    if( m_DebugMode == true )
    {
        if( m_EngineNames.count( name ) == 1 ) //Engine exists
            throw std::invalid_argument( "ERROR: method " + methodName + " already created by Open, in call from Open.\n" );

        if( m_Methods.count( methodName ) == 0 ) //
            throw std::invalid_argument( "ERROR: method " + methodName + " has not been defined, in call from Open\n" );
    }

    ++m_EngineCounter;

    if( methodName.empty() ) //default engine with one transport
    {

    }
    else //special cases
    {
        if( methodName == "SIRIUS" )
        {
            m_Engines[ m_EngineCounter ] =
        }
        else if( methodName == "DataMan" )
        {
            m_Engines[ m_EngineCounter ] = ;
        }
    }

    return m_EngineCounter;
}


void ADIOS::Close( const unsigned int methodHandler, const int transportIndex ) //close stream
{
    auto itEngine = m_Engines.find( methodHandler );
    if( m_DebugMode == true )
    {

    }

    itEngine

    itCapsule->second.Close( transportIndex );
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

        if( Support::Transforms.count( transform ) == 0 )
            throw std::invalid_argument( "ERROR: transform method " + transform +
                                         " not supported, in call to SetTransform\n" );
    }

    int transformIndex, compressionLevel;
    //set m_Transforms, transformIndex, compressionLevel
    SetTransformHelper( transform, m_Transforms, m_DebugMode, transformIndex, compressionLevel );
    //set Variable with transformIndex, compressionLevel
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
void ADIOS::CheckGroup( std::map< std::string, Group >::const_iterator itGroup,
                        const std::string groupName, const std::string hint ) const
{
    if( itGroup == m_Groups.end() )
        throw std::invalid_argument( "ERROR: group " + groupName + " not found " + hint + "\n" );
}

void ADIOS::CheckEngine( std::unordered_map< unsigned int, std::shared_ptr<Engine> >::const_iterator itEngine,
                         const unsigned int handle, const std::string hint ) const
{
    if( itEngine == m_Engines.end() )
        throw std::invalid_argument( "ERROR: stream (or file) from handle " + handle + " not created with Open , " + hint + "\n" );
}


} //end namespace
