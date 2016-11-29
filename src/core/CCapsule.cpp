/*
 * CCapsule.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: wfg
 */

/// \cond EXCLUDE_FROM_DOXYGEN
#include <stdexcept> //std::invalid_argument
#include <cstring> //memcpy
/// \endcond

#include "core/CCapsule.h"
#include "public/SSupport.h"

#ifdef HAVE_BZIP2
#include "transform/CBZIP2.h"
#endif


namespace adios
{

CCapsule::CCapsule( )
{ }


CCapsule::CCapsule( const MPI_Comm mpiComm, const bool debugMode, const std::string streamName,
                    const std::string accessMode, const std::string transport, const std::vector<std::string>& arguments ):
    m_MPIComm{ mpiComm },
    m_DebugMode{ debugMode }
{
   int transportIndex = AddTransport( streamName, accessMode, true, transport, arguments );
}


CCapsule::~CCapsule( )
{ }


int CCapsule::AddTransport( const std::string streamName, const std::string accessMode, const bool isDefault,
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
        m_Transports.push_back( std::make_shared<CPOSIX>( m_MPIComm, m_DebugMode, arguments ) ); //need to add arguments

    else if( transport == "FStream" )
        m_Transports.push_back( std::make_shared<CFStream>( m_MPIComm, m_DebugMode, arguments ) );

    else if( transport == "DataMan" )
        m_Transports.push_back( std::make_shared<CDataMan>( m_MPIComm, m_DebugMode, arguments ) );

}


void CCapsule::Close( int transportIndex )
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
std::string CCapsule::GetName( const std::vector<std::string>& arguments ) const
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
            std::invalid_argument("ERROR: argument to name= is empty in call to AddTransport" );
    }

    return name;
}



//void CCapsule::CreateTransform( const std::string transform )
//{
//    std::string method( transform );
//    auto colonPosition = transform.find(":");
//
//    if( colonPosition != transform.npos )
//    {
//        method = transform.substr( 0, colonPosition );
//    }
//
//    if( m_Transforms.find( method ) != m_Transforms.end() ) //transform method already exists, do nothing
//        return;
//
//    if( method == "bzip2" ) //here must add debug mode exception
//    {
//        #ifdef HAVE_BZIP2
//        m_Transforms["bzip2"] = std::make_shared<CBZIP2>( );
//        #endif
//    }
//}




} //end namespace
