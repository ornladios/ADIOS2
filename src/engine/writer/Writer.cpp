/*
 * SingleBP.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */

#include "engine/writer/Writer.h"
#include "functions/adiosFunctions.h" //GetTotalSize

//supported capsules
#include "capsule/Heap.h"

//supported transports
#include "transport/POSIX.h"
#include "transport/FStream.h"
#include "transport/File.h"

#include "ADIOS.h"

namespace adios
{


Writer::Writer( ADIOS& adios, const std::string name, const std::string accessMode, const MPI_Comm mpiComm,
                const Method& method, const bool debugMode, const unsigned int cores ):
    Engine( adios, "Writer", name, accessMode, mpiComm, method, debugMode, cores, " Writer constructor (or call to ADIOS Open).\n" ),
    m_Buffer{ Heap( accessMode, m_RankMPI, m_DebugMode ) },
    m_MaxBufferSize{ m_Buffer.m_Data.max_size() }
{
    Init( );
}


Writer::~Writer( )
{ }


void Writer::Init( )
{
    auto itGrowthFactor = m_Method.m_Parameters.find( "buffer_growth" );
    if( itGrowthFactor != m_Method.m_Parameters.end() )
    {
        m_GrowthFactor = std::stof( itGrowthFactor->second ); //float
        m_BP1Writer.m_GrowthFactor = m_GrowthFactor;
    }

    auto itMaxBufferSize = m_Method.m_Parameters.find( "max_size_MB" );
    if( itMaxBufferSize != m_Method.m_Parameters.end() )
        m_MaxBufferSize = std::stoul( itGrowthFactor->second ) * 1000000; //convert to bytes

    auto itVerbosity = m_Method.m_Parameters.find( "verbose" );
    if( itVerbosity != m_Method.m_Parameters.end() )
        m_BP1Writer.m_Verbosity = std::stoi( itVerbosity->second );

    InitTransports( );
}


void Writer::Write( Variable<char>& variable, const char* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<unsigned char>& variable, const unsigned char* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<short>& variable, const short* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<unsigned short>& variable, const unsigned short* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<int>& variable, const int* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<unsigned int>& variable, const unsigned int* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<long int>& variable, const long int* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<unsigned long int>& variable, const unsigned long int* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<long long int>& variable, const long long int* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<unsigned long long int>& variable, const unsigned long long int* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<float>& variable, const float* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<double>& variable, const double* values )
{ WriteVariable( variable, values ); }

void Writer::Write( Variable<long double>& variable, const long double* values )
{ WriteVariable( variable, values ); }

//String version
void Writer::Write( const std::string variableName, const char* values )
{ WriteVariable( m_ADIOS.GetVariable<char>( variableName ), values ); }

void Writer::Write( const std::string variableName, const unsigned char* values )
{ WriteVariable( m_ADIOS.GetVariable<unsigned char>( variableName ), values ); }

void Writer::Write( const std::string variableName, const short* values )
{ WriteVariable( m_ADIOS.GetVariable<short>( variableName ), values ); }

void Writer::Write( const std::string variableName, const unsigned short* values )
{ WriteVariable( m_ADIOS.GetVariable<unsigned short>( variableName ), values ); }

void Writer::Write( const std::string variableName, const int* values )
{ WriteVariable( m_ADIOS.GetVariable<int>( variableName ), values ); }

void Writer::Write( const std::string variableName, const unsigned int* values )
{ WriteVariable( m_ADIOS.GetVariable<unsigned int>( variableName ), values ); }

void Writer::Write( const std::string variableName, const long int* values )
{ WriteVariable( m_ADIOS.GetVariable<long int>( variableName ), values ); }

void Writer::Write( const std::string variableName, const unsigned long int* values )
{ WriteVariable( m_ADIOS.GetVariable<unsigned long int>( variableName ), values ); }

void Writer::Write( const std::string variableName, const long long int* values )
{ WriteVariable( m_ADIOS.GetVariable<long long int>( variableName ), values ); }

void Writer::Write( const std::string variableName, const unsigned long long int* values )
{ WriteVariable( m_ADIOS.GetVariable<unsigned long long int>( variableName ), values ); }

void Writer::Write( const std::string variableName, const float* values )
{ WriteVariable( m_ADIOS.GetVariable<float>( variableName ), values ); }

void Writer::Write( const std::string variableName, const double* values )
{ WriteVariable( m_ADIOS.GetVariable<double>( variableName ), values ); }

void Writer::Write( const std::string variableName, const long double* values )
{ WriteVariable( m_ADIOS.GetVariable<long double>( variableName ), values ); }




void Writer::Close( const int transportIndex )
{
    //BP1Writer to update the metadata indices


    //merge all metadata indices in capsule.m_Metadata buffer or capsule.m_Data buffer (depends on transport predefined functionality)


    //BP1Writer to write to corresponding transport

    if( transportIndex == -1 ) // all transports
    {
        for( auto& transport : m_Transports )
            transport->Write( m_Buffer.m_Data.data(), m_Buffer.m_DataPosition );

        for( auto& transport : m_Transports )
            transport->Close( );   //actually no need, close is in destructor (like fstream)
    }
    else
    {
        m_Transports[ transportIndex ]->Write( m_Buffer.m_Data.data(), m_Buffer.m_DataPosition );
    }

    //Close the corresponding transport
}





void Writer::InitTransports( )
{
    if( m_DebugMode == true )
    {
        if( TransportNamesUniqueness( ) == false )
        {
            throw std::invalid_argument( "ERROR: two transports of the same kind (e.g file IO) "
                                         "can't have the same name, modify with name= in Method AddTransport\n" );
        }
    }

    for( const auto& parameters : m_Method.m_TransportParameters )
    {
        auto itTransport = parameters.find( "transport" );

        if( itTransport->second == "POSIX" )
        {
            auto file = std::make_shared<POSIX>( m_MPIComm, m_DebugMode );
            m_BP1Writer.OpenRankFiles( m_Name, m_AccessMode, *file );
            m_Transports.push_back( std::move( file ) );
        }
        else if( itTransport->second == "File" )
        {
            auto file = std::make_shared<File>( m_MPIComm, m_DebugMode );
            m_BP1Writer.OpenRankFiles( m_Name, m_AccessMode, *file );
            m_Transports.push_back( std::move( file ) );
        }
        else if( itTransport->second == "FStream" )
        {
            auto file = std::make_shared<FStream>( m_MPIComm, m_DebugMode );
            m_BP1Writer.OpenRankFiles( m_Name, m_AccessMode, *file );
            m_Transports.push_back( std::move( file ) );
        }
        else if( itTransport->second == "MPIFile" ) //not yet supported
        {
            //m_Transports.push_back( std::make_shared<MPIFile>( m_MPIComm, m_DebugMode ) );
        }
        else
        {
            if( m_DebugMode == true )
                throw std::invalid_argument( "ERROR: transport + " + itTransport->second + " not supported, in " +
                                              m_Name + m_EndMessage );
        }
    }
}



bool Writer::CheckBuffersAllocation( const std::string variableName, const std::size_t indexSize,
                                     const std::size_t payloadSize )
{
    //Check if data in buffer needs to be reallocated
    const std::size_t dataSize = payloadSize + indexSize + 10; //adding some bytes tolerance
    const std::size_t neededSize = dataSize + m_Buffer.m_DataPosition;
    // might need to write payload in batches
    bool doTransportsFlush = ( neededSize > m_MaxBufferSize )? true : false;

    if( GrowBuffer( m_MaxBufferSize, m_GrowthFactor, m_Buffer.m_DataPosition, m_Buffer.m_Data ) == -1 )
        doTransportsFlush = true;

    GrowBuffer( indexSize, m_GrowthFactor, m_MetadataSet.VarsIndexPosition, m_MetadataSet.VarsIndex ); //not checking for metadata
    return doTransportsFlush;
}



} //end namespace adios

