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
                                MPI_Comm mpiComm, const std::string methodName, const unsigned int cores )
{
    auto itMethod = m_Methods.find( methodName );

    if( m_DebugMode == true )
    {
        CheckMethod( itMethod, methodName, " from call to Open\n" );

        if( m_EngineNames.count( name ) == 1 ) //Check if Engine already exists
            throw std::invalid_argument( "ERROR: engine name " + name + " already created by Open, in call from Open.\n" );
    }

    ++m_EngineCounter;

    if( methodName == "SingleBP" )
    {
        m_Engines[ m_EngineCounter ] = std::make_shared<SingleBP>( name, accessMode, mpiComm, itMethod->second, cores );
    }
    else if( methodName == "SIRIUS" )
    {
        //here must complete
    }
    else if( methodName == "DataMan" )
    {
        //here must complete
    }

    return m_EngineCounter;
}


void ADIOS::Close( const unsigned int handler, const int transportIndex ) //close stream
{
    auto itEngine = m_Engines.find( handler );
    if( m_DebugMode == true )
        CheckEngine( itEngine, handler, " in call to Close\n" );

    itEngine->second->Close( transportIndex );
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


void ADIOS::AddTransform( const std::string groupName, const std::string variableName, const std::string transform )
{
    auto itGroup = m_Groups.find( groupName );
    if( m_DebugMode == true ) //check group and transform
    {
        CheckGroup( itGroup, groupName, " from call to SetTransform \n" );

        if( Support::Transforms.count( transform ) == 0 )
            throw std::invalid_argument( "ERROR: transform method " + transform +
                                         " not supported, in call to SetTransform\n" );
    }

    std::vector<std::string> transformName = { transform };
    std::vector<short> transformIndices, parameters;
    SetTransformsHelper( transformName, m_Transforms, m_DebugMode, transformIndices, parameters );
    itGroup->second.AddTransform( variableName, *m_Transforms[ transformIndices[0] ], parameters[0] );
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


void ADIOS::CheckMethod( std::map< std::string, Method >::const_iterator itMethod,
                         const std::string methodName, const std::string hint ) const
{
    if( itMethod == m_Methods.end() )
        throw std::invalid_argument( "ERROR: method " + methodName + " not found " + hint + "\n" );
}


void ADIOS::CheckEngine( std::unordered_map< unsigned int, std::shared_ptr<Engine> >::const_iterator itEngine,
                         const unsigned int handle, const std::string hint ) const
{
    if( itEngine == m_Engines.end() )
        throw std::invalid_argument( "ERROR: stream (or file) from handle " + handle + " not created with Open , " + hint + "\n" );
}


} //end namespace
