/*
 * CCapsule.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: wfg
 */

#include "core/CCapsule.h"

#ifdef HAVE_BZIP2
#include "transform/CBZIP2.h"
#endif


//transports
#include "transport/CPOSIX.h"
#include "transport/CFStream.h"


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


void CCapsule::SetTransform( const std::string transform )
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


void CCapsule::SetTransport( const std::string streamName, const std::string transport, const bool debugMode )
{
    if( transport == "POSIX" )
        m_Transports[streamName] = std::make_shared<CPOSIX>( m_MPIComm, debugMode );

    else if( transport == "FStream" )
        m_Transports[streamName] = std::make_shared<CFStream>( m_MPIComm, debugMode );

    else if( transport == "DataMan" )
        m_Transports[streamName] = std::make_shared<CDataMan>( m_MPIComm, debugMode );

}


void CCapsule::SetBuffer( const std::string streamName, const unsigned int long maxBufferSize )
{
    auto itBuffer = m_Buffer.find( streamName );

    if( m_DebugMode == true )
    {
        if( itBuffer == m_Buffer.end() )
            throw std::invalid_argument( "ERROR: buffer for stream " + streamName + " not found, not setting size\n" );
    }

    itBuffer->second.resize( maxBufferSize * 1000000 ); //use resize not reserve
}


void CCapsule::Open( const std::string streamName, const std::string accessMode )
{
    m_Transports[streamName]->Open( streamName, accessMode );
    m_Transports[streamName]->SetBuffer( m_Buffer[streamName] );
}


void CCapsule::WriteVariableToBuffer( const CGroup& group, const std::string variableName )
{
    const std::string streamName( group.m_StreamName );
    std::vector<unsigned long long int> dimensions = group.GetDimensions( variableName );
    const auto itVariable = group.m_Variables.find( variableName );
    const std::string type( itVariable->second.first );
    const unsigned int index( itVariable->second.second );

    if( type == "char" || type == "character" )
        group.m_Char[index];

    else if( type == "unsigned char" )
        group.m_UChar[index];

    else if( type == "short" || type == "integer*2" )
        group.m_Short[index];

    else if( type == "unsigned short" )
        group.m_UShort[index];

    else if( type == "int" || type == "integer" )
        group.m_Int[index];

    else if( type == "unsigned int" || type == "unsigned integer" )
        group.m_UInt[index];

    else if( type == "long int" || type == "long" || type == "long integer" )
        group.m_LInt[index];

    else if( type == "long long int" || type == "long long" || type == "long long integer" )
        group.m_LLInt[index];

    else if( type == "unsigned long long int" || type == "unsigned long long" || type == "unsigned long long integer" )
        group.m_ULLInt[index];

    else if( type == "float" || type == "real" || type == "real*4" )
        group.m_Float[index];

    else if( type == "double" || type == "double precision" || type == "real*8" )
        group.m_Double[index];


    else if( type == "long double" || type == "real*16" )
        m_LDouble.push_back( SVariable<long double>{ dimensionsCSV, nullptr, transformIndex, globalBoundsIndex } );







    if( group.m_Transport == "DataMan" ) // send a single buffer to DataMan with the entire data
    {


    }






}


void CCapsule::CloseBuffer( const std::string streamName )
{
    m_Transports[streamName]->Close( );
}



} //end namespace
