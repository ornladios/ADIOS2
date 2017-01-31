/*
 * Engine.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */


#include "core/Engine.h"
#include "core/Support.h"
#include "functions/adiosFunctions.h"


namespace adios
{


Engine::Engine( const std::string engineType, const std::string name, const std::string accessMode,
                const MPI_Comm mpiComm, const Method& method,
                const bool debugMode, const unsigned int cores, const std::string endMessage ):
    m_MPIComm{ mpiComm },
    m_EngineType{ engineType },
    m_Name{ name },
    m_AccessMode{ accessMode },
    m_Method{ method },
    m_Group{ method.m_Group },
    m_DebugMode{ debugMode },
    m_Cores{ cores },
    m_EndMessage{ endMessage }
{
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
}


Engine::~Engine( )
{ }

void Engine::SetDefaultGroup( Group& group )
{
    m_Group = &group;
}


//PROTECTED
unsigned int Engine::PreSetVariable( Group& group, const std::string variableName,
                                     const std::string hint )
{
    auto itVariable = group.m_Variables.find( variableName );

    if( m_DebugMode == true )
    {
        if( itVariable == group.m_Variables.end() )
            throw std::invalid_argument( "ERROR: variable " + variableName + " doesn't exist " + hint + ".\n" );
    }

    group.m_WrittenVariables.insert( variableName ); // group tracks its own written variables for dimensions
    m_WrittenVariables.push_back( std::make_pair( &group, variableName ) );
    const unsigned int index = itVariable->second.second;
    return index;
}


void Engine::Close( int transportIndex )
{
    if( transportIndex == -1 ) //close all transports
    {
        for( auto& transport : m_Transports )
            transport->Close( );
    }
    else
    {
        m_Transports[transportIndex]->Close( ); //here need to pass the buffer
    }
}

void Engine::Init( )
{ }


void Engine::InitCapsules( )
{ }


void Engine::InitTransports( )
{ }


void Engine::CheckParameter( const std::map<std::string, std::string>::const_iterator itParam,
                             const std::map<std::string, std::string>& parameters,
                             const std::string parameterName, const std::string hint ) const
{
    if( itParam == parameters.end() )
        throw std::invalid_argument( "ERROR: parameter name " + parameterName + " not found " + hint );
}



void Engine::CheckDefaultGroup( ) const
{
    if( m_DebugMode == true )
    {
        if( m_Group == nullptr )
            throw std::invalid_argument( "ERROR: default group in engine " + m_Name + " is nullptr, check Method\n" );
    }
}


std::string Engine::GetName( const std::vector<std::string>& arguments ) const
{
    bool isNameFound = false;
    std::string name;

    for( const auto argument : arguments )
    {
        auto namePosition = argument.find( "name=" );
        if( namePosition != argument.npos )
        {
            isNameFound = true;
            name = argument.substr( namePosition + 5 );
            break;
        }
    }

    if( m_DebugMode == true )
    {
        if( name.empty() || isNameFound == false )
            std::invalid_argument( "ERROR: argument to name= is empty or not found in call to AddTransport" );
    }

    return name;
}


} //end namespace
