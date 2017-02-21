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


Engine::Engine( ADIOS& adios, const std::string engineType, const std::string name, const std::string accessMode,
                const MPI_Comm mpiComm, const Method& method, const bool debugMode, const unsigned int cores,
                const std::string endMessage ):
    m_MPIComm{ mpiComm },
    m_EngineType{ engineType },
    m_Name{ name },
    m_AccessMode{ accessMode },
    m_Method{ method },
    m_ADIOS{ adios },
    m_DebugMode{ debugMode },
    m_Cores{ cores },
    m_EndMessage{ endMessage }
{
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
}


Engine::~Engine( )
{ }


//PROTECTED

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


bool Engine::TransportNamesUniqueness( ) const
{
    auto lf_CheckTransportsType = [&]( const std::set<std::string>& specificType ) -> bool
    {
        std::set<std::string> transportNames;

        for( const auto& parameters : m_Method.m_TransportParameters )
        {
            auto itTransport = parameters.find( "transport" );
            if( m_DebugMode == true )
            {
                if( itTransport == parameters.end() )
                    throw std::invalid_argument( "ERROR: transport not defined in Method input to Engine " + m_Name );
            }

            const std::string type( itTransport->second );
            if( specificType.count( type ) == 1   ) //file transports type
            {
                std::string name( m_Name );
                auto itName = parameters.find("name");
                if( itName != parameters.end() )
                    name = itName->second;

                if( transportNames.count( name ) == 0 )
                    transportNames.insert( name );
                else
                    return false;
            }
        }
        return true;
    };

    return lf_CheckTransportsType( Support::FileTransports );
}


} //end namespace
