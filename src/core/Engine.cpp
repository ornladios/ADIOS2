/*
 * Engine.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */


#include "core/Engine.h"


namespace adios
{


Engine::Engine( const std::string engineType, const std::string name, const std::string accessMode,
                const MPI_Comm mpiComm, const Method& method,
                const bool debugMode ):
    m_EngineType{ engineType },
    m_Name{ name },
    m_AccessMode{ accessMode },
    m_Method{ &method },
    m_MPIComm{ mpiComm }
{
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
}


Engine::~Engine( )
{ }


void Engine::Close( int transportIndex )
{
    if( transportIndex == -1 ) //close all transports
    {
        for( auto& transport : m_Transports )
            transport->Close( );
    }
    else
    {
        m_Transports[transportIndex]->Close( );
    }
}


//PROTECTED FUNCTIONS
void Engine::SetTransports( )
{
    for( const auto& transportPair : m_Method->Transports )
    {
        const std::string transport = transportPair.first;
        const std::vector<std::string>& arguments = transportPair.second;

        if( transport == "POSIX" )
            m_Transports.push_back( std::make_shared<POSIX>( m_MPIComm, m_DebugMode, arguments ) );

        else if( transport == "FStream" )
            m_Transports.push_back( std::make_shared<FStream>( m_MPIComm, m_DebugMode, arguments ) );

        else
        {
            if( m_DebugMode == true )
                throw std::invalid_argument( "ERROR: transport + " + transport + " not supported, in call from Open.\n" );
        }

        std::string name = GetName( arguments );
        m_Transports.back()->Open( name, m_AccessMode );
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



