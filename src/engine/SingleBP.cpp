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
        if( m_Method.Capsules.size() != 1 )
            throw std::invalid_argument( "ERROR: SingleBP engine only allows one heap buffer, from SingleBP constructor in Open.\n" );

        if( m_Method.Capsules[0] != "Heap" )
            throw std::invalid_argument( "ERROR: SingleBP doesn't support Capsule of type " + m_Method.Capsules[0] + ", from SingleBP constructor in Open.\n" );
    }
    //Create single capsule of type heap
    m_Capsules.push_back( std::make_shared<Heap>( m_AccessMode, m_RankMPI, m_Cores ) );
}



void SingleBP::InitTransports( )
{
    std::set< std::string > transportStreamNames; //used to check for name conflict between transports
    std::string name = GetName( arguments );
    m_Transports.back()->Open( name, m_AccessMode );

    for( const auto& transportPair : m_Method.Transports )
    {
        const std::string transport = transportPair.first;
        const std::vector<std::string>& arguments = transportPair.second;

        if( transport == "POSIX" )
        {
            m_Transports.push_back( std::make_shared<POSIX>( m_MPIComm, m_DebugMode, arguments ) );
        }
        else if( transport == "FStream" )
        {
            m_Transports.push_back( std::make_shared<FStream>( m_MPIComm, m_DebugMode, arguments ) );
        }
        else
        {
            if( m_DebugMode == true )
                throw std::invalid_argument( "ERROR: transport + " + transport + " not supported, in Engine constructor (or Open).\n" );
        }


    }
}












} //end namespace

