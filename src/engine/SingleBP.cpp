/*
 * SingleBP.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */
#include "engine/SingleBP.h"

//supported capsules
#include "capsule/Heap.h"

//supported transports
#include "transport/POSIX.h"
#include "transport/FStream.h"


namespace adios
{


SingleBP::SingleBP( const std::string streamName, const std::string accessMode, const MPI_Comm mpiComm,
                    const Method& method, const bool debugMode, const unsigned int cores ):
    Engine( "SingleBP", streamName, accessMode, mpiComm, method, debugMode, cores )
{
    Init( );
}


SingleBP::~SingleBP( )
{ }


void SingleBP::Init( )
{
    InitCapsules( );
    InitTransports( );
}


void SingleBP::Write( Group& group, const std::string variableName, const double* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("double"), " from call to Write double*" );


}


void SingleBP::InitCapsules( )
{
    if( m_DebugMode == true )
    {
        if( m_Method.m_CapsuleParameters.size() > 1 )
        {
            throw std::invalid_argument( "ERROR: SingleBP engine only allows one heap buffer, from SingleBP constructor in Open.\n" );
        }
        else if( m_Method.m_CapsuleParameters.size() == 1 )
        {
            if( m_Method.m_CapsuleParameters[0].at("type") != "Heap" )
                throw std::invalid_argument( "ERROR: SingleBP doesn't support Capsule of type " +  + ", from SingleBP constructor in Open.\n" );
        }

    }
    //Create single capsule of type heap
    m_Capsules.push_back( std::make_shared<Heap>( m_AccessMode, m_RankMPI, m_Cores ) );
}



void SingleBP::InitTransports( )
{
    std::set< std::string > transportStreamNames; //used to check for name conflict between transports


    for( const auto& parameters : m_Method.m_TransportParameters )
    {
        const std::string transport = parameters.at("type");
        const std::string name = parameters.at("name");

        if( transport == "POSIX" )
        {
            m_Transports.push_back( std::make_shared<POSIX>( m_MPIComm, m_DebugMode ) );
        }
        else if( transport == "File" )
        {
            m_Transports.push_back( std::make_shared<FStream>( m_MPIComm, m_DebugMode ) );

        }
        else if( transport == "FStream" )
        {
            m_Transports.push_back( std::make_shared<FStream>( m_MPIComm, m_DebugMode ) );
        }
        else if( transport == "MPIFile" )
        {
            m_Transports.push_back( std::make_shared<FStream>( m_MPIComm, m_DebugMode ) );
        }
        else
        {
            if( m_DebugMode == true )
                throw std::invalid_argument( "ERROR: transport + " + transport + " not supported, in Engine constructor (or Open).\n" );
        }


        m_Transports.back()->Open( name, m_AccessMode );
    }
}












} //end namespace

