/*
 * Capsule.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: wfg
 */

/// \cond EXCLUDE_FROM_DOXYGEN
#include <stdexcept> //std::invalid_argument
#include <cstring> //memcpy
/// \endcond


#ifdef HAVE_BZIP2
#include "transform/BZIP2.h"
#endif

#include "../../include/core/Engine.h"
#include "../../include/core/Support.h"

//transport below
#include "transport/POSIX.h"
#include "transport/FStream.h"


namespace adios
{


Capsule::Capsule( )
{ }


Capsule::Capsule( const MPI_Comm mpiComm, const bool debugMode, const std::string streamName,
                    const std::string accessMode, const std::string transport, const std::vector<std::string>& arguments ):
    m_MPIComm{ mpiComm },
    m_DebugMode{ debugMode }
{
   AddTransport( streamName, accessMode, true, transport, arguments );
}


Capsule::~Capsule( )
{ }


int Capsule::AddTransport( const std::string streamName, const std::string accessMode, const bool isDefault,
                            const std::string transport, const std::vector<std::string>& arguments )
{

    std::string name( streamName ); //default name to transport
    if( isDefault == false ) //adding more than default
    {
        name = GetName( arguments );
        if( m_DebugMode == true )
        {
            if( name == streamName )
                throw std::invalid_argument( "ERROR: name= argument can't be the same as stream (or file ) from Open, in call to AddTransport\n" );
        }
    }

    if( transport == "POSIX" )
        m_Transports.push_back( std::make_shared<POSIX>( m_MPIComm, m_DebugMode, arguments ) );

    else if( transport == "FStream" )
        m_Transports.push_back( std::make_shared<FStream>( m_MPIComm, m_DebugMode, arguments ) );

    int transportIndex = static_cast<int>( m_Transports.size() - 1 );
    m_Transports[ transportIndex ]->Open( name, accessMode );

    return transportIndex;
}


void Capsule::Close( int transportIndex )
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


//PRIVATE FUNCTIONS
std::string Capsule::GetName( const std::vector<std::string>& arguments ) const
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
