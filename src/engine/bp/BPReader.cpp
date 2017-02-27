/*
 * BPReader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: wfg
 */



#include "engine/bp/BPReader.h"

#include "core/Support.h"
#include "functions/adiosFunctions.h" //CSVToVector

//supported transports
#include "transport/file/FD.h" // uses POSIX
#include "transport/file/FP.h" // uses C FILE*
#include "transport/file/FStream.h" // uses C++ fstream


namespace adios
{

BPReader::BPReader( ADIOS& adios, const std::string name, const std::string accessMode, MPI_Comm mpiComm,
                    const Method& method, const bool debugMode, const unsigned int cores ):
    Engine( adios, "BPReader", name, accessMode, mpiComm, method, debugMode, cores, " BPReader constructor (or call to ADIOS Open).\n" ),
    m_Buffer( accessMode, m_RankMPI, m_DebugMode )
{
    Init( );
}

BPReader::~BPReader( )
{ }


Variable<void>* BPReader::InquireVariable( const std::string name, const bool readIn ) //not yet implemented
{ return nullptr; }

Variable<char>* BPReader::InquireVariableChar( const std::string name, const bool readIn )
{ return InquireVariableCommon<char>( name, readIn ); }

Variable<unsigned char>* BPReader::InquireVariableUChar( const std::string name, const bool readIn )
{ return InquireVariableCommon<unsigned char>( name, readIn ); }

Variable<short>* BPReader::InquireVariableShort( const std::string name, const bool readIn )
{ return InquireVariableCommon<short>( name, readIn ); }

Variable<unsigned short>* BPReader::InquireVariableUShort( const std::string name, const bool readIn )
{ return InquireVariableCommon<unsigned short>( name, readIn ); }

Variable<int>* BPReader::InquireVariableInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<int>( name, readIn ); }

Variable<unsigned int>* BPReader::InquireVariableUInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<unsigned int>( name, readIn ); }

Variable<long int>* BPReader::InquireVariableLInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<long int>( name, readIn ); }

Variable<unsigned long int>* BPReader::InquireVariableULInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<unsigned long int>( name, readIn ); }

Variable<long long int>* BPReader::InquireVariableLLInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<long long int>( name, readIn ); }

Variable<unsigned long long int>* BPReader::InquireVariableULLInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<unsigned long long int>( name, readIn ); }

Variable<float>* BPReader::InquireVariableFloat( const std::string name, const bool readIn )
{ return InquireVariableCommon<float>( name, readIn ); }

Variable<double>* BPReader::InquireVariableDouble( const std::string name, const bool readIn )
{ return InquireVariableCommon<double>( name, readIn ); }

Variable<long double>* BPReader::InquireVariableLDouble( const std::string name, const bool readIn )
{ return InquireVariableCommon<long double>( name, readIn ); }

Variable<std::complex<float>>* BPReader::InquireVariableCFloat( const std::string name, const bool readIn )
{ return InquireVariableCommon<std::complex<float>>( name, readIn ); }

Variable<std::complex<double>>* BPReader::InquireVariableCDouble( const std::string name, const bool readIn )
{ return InquireVariableCommon<std::complex<double>>( name, readIn ); }

Variable<std::complex<long double>>* BPReader::InquireVariableCLDouble( const std::string name, const bool readIn )
{ return InquireVariableCommon<std::complex<long double>>( name, readIn ); }

VariableCompound* BPReader::InquireVariableCompound( const std::string name, const bool readIn )
{ return nullptr; }


void BPReader::Close( const int transportIndex )
{

}


//PRIVATE
void BPReader::Init( )
{
    if( m_DebugMode == true )
    {
        if( m_AccessMode != "r" && m_AccessMode != "read" )
            throw std::invalid_argument( "ERROR: BPReader doesn't support access mode " + m_AccessMode +
                                         ", in call to ADIOS Open or BPReader constructor\n"  );
    }

    InitCapsules( );
    InitTransports( );
}


void BPReader::InitCapsules( )
{
    //here init memory capsules
}


void BPReader::InitTransports( ) //maybe move this?
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
        if( itTransport->second == "file" || itTransport->second == "File" )
        {
            auto itLibrary = parameters.find( "library" );
            if( itLibrary == parameters.end() || itLibrary->second == "POSIX" ) //use default POSIX
            {
                auto file = std::make_shared<transport::FD>( m_MPIComm, m_DebugMode );
                //m_BP1Writer.OpenRankFiles( m_Name, m_AccessMode, *file );
                m_Transports.push_back( std::move( file ) );
            }
            else if( itLibrary->second == "FILE*" || itLibrary->second == "stdio.h" )
            {
                auto file = std::make_shared<transport::FP>( m_MPIComm, m_DebugMode );
                //m_BP1Writer.OpenRankFiles( m_Name, m_AccessMode, *file );
                m_Transports.push_back( std::move( file ) );

            }
            else if( itLibrary->second == "fstream" || itLibrary->second == "std::fstream" )
            {
                auto file = std::make_shared<transport::FStream>( m_MPIComm, m_DebugMode );
                //m_BP1Writer.OpenRankFiles( m_Name, m_AccessMode, *file );
                m_Transports.push_back( std::move( file ) );
            }
            else if( itLibrary->second == "MPI-IO" )
            {

            }
            else
            {
                if( m_DebugMode == true )
                    throw std::invalid_argument( "ERROR: file transport library " + itLibrary->second + " not supported, in " +
                            m_Name + m_EndMessage );
            }
        }
        else
        {
            if( m_DebugMode == true )
                throw std::invalid_argument( "ERROR: transport " + itTransport->second + " (you mean File?) not supported, in " +
                        m_Name + m_EndMessage );
        }
    }
}


} //end namespace


