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

#ifdef HAVE_BZIP2
#include "transform/CBZIP2.h"
#endif


namespace adios
{

CCapsule::CCapsule( )
{ }


CCapsule::CCapsule( const bool debugMode ):
    m_DebugMode{ debugMode }
{ }


CCapsule::CCapsule( MPI_Comm mpiComm, const bool debugMode ):
    m_MPIComm{ mpiComm },
    m_DebugMode{ debugMode }
{
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
}


CCapsule::~CCapsule( )
{ }



void CCapsule::Close( const std::string streamName )
{
    m_Streams[streamName].Transport->Close( ); //should release resources
}

//PRIVATE FUNCTIONS
void CCapsule::CreateTransport( const std::string streamName, const std::string transport )
{

}


void CCapsule::CreateBuffer( const std::string streamName, const size_t maxBufferSize )
{
    m_Buffers[streamName] = std::vector<char>( 1 ); // vector of size 1
    m_MaxBufferSize[streamName] = maxBufferSize;
}


void CCapsule::CreateTransform( const std::string transform )
{
    std::string method( transform );
    auto colonPosition = transform.find(":");

    if( colonPosition != transform.npos )
    {
        method = transform.substr( 0, colonPosition );
    }

    if( m_Transforms.find( method ) != m_Transforms.end() ) //transform method already exists, do nothing
        return;

    if( method == "bzip2" ) //here must add debug mode exception
    {
        #ifdef HAVE_BZIP2
        m_Transforms["bzip2"] = std::make_shared<CBZIP2>( );
        #endif
    }
}




} //end namespace
