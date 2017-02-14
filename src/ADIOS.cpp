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

//Engines
#include "engine/writer/Writer.h"
#include "engine/dataman/DataMan.h"
#include "engine/vis/Vis.h"


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


Group& ADIOS::DeclareGroup( const std::string groupName )
{
    if( m_DebugMode == true )
    {
        if( m_Groups.count( groupName ) == 1 )
            throw std::invalid_argument( "ERROR: group " + groupName + " already exist, from call to DeclareGroup\n" );
    }

    m_Groups.emplace( groupName, Group( groupName, m_DebugMode ) );
    return m_Groups.at( groupName );
}


Method& ADIOS::DeclareMethod( const std::string methodName, const std::string type )
{
    if( m_DebugMode == true )
    {
        if( m_Methods.count( methodName ) == 1 )
            throw std::invalid_argument( "ERROR: method " + methodName + " already declared, from DeclareMethod\n" );
    }
    m_Methods.emplace( methodName, Method( type, m_DebugMode ) );
    return m_Methods.at( methodName );
}


std::shared_ptr<Engine> ADIOS::Open( const std::string name, const std::string accessMode, MPI_Comm mpiComm, const Method& method, const unsigned int cores )
{
    if( m_DebugMode == true )
    {
        if( m_EngineNames.count( name ) == 1 ) //Check if Engine already exists
            throw std::invalid_argument( "ERROR: engine name " + name + " already created by Open, in call from Open.\n" );
    }

    m_EngineNames.insert( name );

    const std::string type( method.m_Type );

    if( type == "Writer" || type == "writer" )
    {
        return std::make_shared<Writer>( name, accessMode, mpiComm, method, m_DebugMode, cores, m_HostLanguage );
    }
    else if( type == "SIRIUS" || type == "sirius" || type == "Sirius" )
    {
        //not yet supported
        //return std::make_shared<engine::DataMan>( name, accessMode, mpiComm, method, m_DebugMode, cores, m_HostLanguage );
    }
    else if( type == "DataMan" )
    {
    	return std::make_shared<engine::DataMan>( name, accessMode, mpiComm, method, m_DebugMode, cores, m_HostLanguage );
    }
    else if( type == "Vis" )
    {
        return std::make_shared<engine::Vis>( name, accessMode, mpiComm, method, m_DebugMode, cores, m_HostLanguage );
    }
    else
    {
        if( m_DebugMode == true )
            throw std::invalid_argument( "ERROR: method type " + type + " not supported for " + name + ", in call to Open\n" );
    }

    return nullptr; // if debug mode is off
}


std::shared_ptr<Engine> ADIOS::Open( const std::string streamName, const std::string accessMode, const Method& method,
                                     const unsigned int cores )
{
    return Open( streamName, accessMode, m_MPIComm, method, cores );
}


std::shared_ptr<Engine> ADIOS::Open( const std::string name, const std::string accessMode, MPI_Comm mpiComm,
                                     const std::string methodName, const unsigned int cores )
{
    auto itMethod = m_Methods.find( methodName );

    if( m_DebugMode == true )
    {
        CheckMethod( itMethod, methodName, " in call to Open\n" );
    }

    return Open( name, accessMode, mpiComm, itMethod->second, cores );
}


std::shared_ptr<Engine> ADIOS::Open( const std::string name, const std::string accessMode,
                                     const std::string methodName, const unsigned int cores )
{
    return Open( name, accessMode, m_MPIComm, methodName, cores );
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
void ADIOS::CheckVariable( const std::string name, const Dims& dimensions ) const
{
    if( m_DebugMode == true )
    {
        if( m_Variables.count( name ) == 1 )
            throw std::invalid_argument( "ERROR: variable " + name + " already exists, in call to DefineVariable\n" );

        if( dimensions.empty() == true )
            throw std::invalid_argument( "ERROR: variable " + name + " dimensions can't be empty, in call to DefineVariable\n" );
    }
}


void ADIOS::CheckMethod( std::map< std::string, Method >::const_iterator itMethod,
                         const std::string methodName, const std::string hint ) const
{
    if( itMethod == m_Methods.end() )
        throw std::invalid_argument( "ERROR: method " + methodName + " not found " + hint + "\n" );
}


} //end namespace
