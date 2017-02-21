/*
 * DataMan.cpp
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#include <iostream> //needs to go away, this is just for demo purposes

#include "engine/dataman/DataManWriter.h"

#include "core/Support.h"
#include "functions/adiosFunctions.h" //CSVToVector

//supported transports
#include "transport/file/FD.h" // uses POSIX
#include "transport/file/FP.h" // uses C FILE*
#include "transport/file/FStream.h" // uses C++ fstream
#include "transport/wan/MdtmMan.h" //uses Mdtm library


namespace adios
{


DataManWriter::DataManWriter( ADIOS& adios, const std::string name, const std::string accessMode, MPI_Comm mpiComm,
                              const Method& method, const bool debugMode, const unsigned int cores ):
    Engine( adios, "DataManWriter", name, accessMode, mpiComm, method, debugMode, cores, " Dataman constructor (or call to ADIOS Open).\n" ),
    m_Buffer( accessMode, m_RankMPI, m_DebugMode )
{
    Init( );
}


DataManWriter::~DataManWriter( )
{ }


void DataManWriter::Init( )
{
    InitCapsules( );
    InitTransports( );
}


void DataManWriter::Write( Variable<char>& variable, const char* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<unsigned char>& variable, const unsigned char* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<short>& variable, const short* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<unsigned short>& variable, const unsigned short* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<int>& variable, const int* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<unsigned int>& variable, const unsigned int* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<long int>& variable, const long int* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<unsigned long int>& variable, const unsigned long int* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<long long int>& variable, const long long int* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<unsigned long long int>& variable, const unsigned long long int* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<float>& variable, const float* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<double>& variable, const double* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( Variable<long double>& variable, const long double* values )
{ WriteVariable( variable, values ); }

void DataManWriter::Write( const std::string variableName, const char* values )
{ WriteVariable( m_ADIOS.GetVariable<char>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const unsigned char* values )
{ WriteVariable( m_ADIOS.GetVariable<unsigned char>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const short* values )
{ WriteVariable( m_ADIOS.GetVariable<short>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const unsigned short* values )
{ WriteVariable( m_ADIOS.GetVariable<unsigned short>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const int* values )
{ WriteVariable( m_ADIOS.GetVariable<int>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const unsigned int* values )
{ WriteVariable( m_ADIOS.GetVariable<unsigned int>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const long int* values )
{ WriteVariable( m_ADIOS.GetVariable<long int>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const unsigned long int* values )
{ WriteVariable( m_ADIOS.GetVariable<unsigned long int>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const long long int* values )
{ WriteVariable( m_ADIOS.GetVariable<long long int>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const unsigned long long int* values )
{ WriteVariable( m_ADIOS.GetVariable<unsigned long long int>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const float* values )
{ WriteVariable( m_ADIOS.GetVariable<float>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const double* values )
{ WriteVariable( m_ADIOS.GetVariable<double>( variableName ), values ); }

void DataManWriter::Write( const std::string variableName, const long double* values )
{ WriteVariable( m_ADIOS.GetVariable<long double>( variableName ), values ); }


void DataManWriter::InitCapsules( )
{
    //here init memory capsules
}


void DataManWriter::InitTransports( ) //maybe move this?
{
    TransportNamesUniqueness( );

    for( const auto& parameters : m_Method.m_TransportParameters )
    {
        auto itTransport = parameters.find( "transport" );

        if( itTransport->second == "Mdtm" || itTransport->second == "MdtmMan" )
        {
            const std::string localIP( GetMdtmParameter( "localIP", parameters ) ); //mandatory
            const std::string remoteIP( GetMdtmParameter( "remoteIP", parameters ) ); //mandatory
            const std::string prefix( GetMdtmParameter( "prefix", parameters ) );
            const int numberOfPipes = std::stoi( GetMdtmParameter( "pipes", parameters ) );
            const std::vector<int> tolerances = CSVToVectorInt( GetMdtmParameter( "tolerances", parameters ) );
            const std::vector<int> priorities = CSVToVectorInt( GetMdtmParameter( "priorities", parameters ) );

            m_Transports.push_back( std::make_shared<transport::MdtmMan>( localIP, remoteIP, m_AccessMode, prefix, numberOfPipes,
                                                                          tolerances, priorities, m_MPIComm, m_DebugMode ) );
        }
        else if( itTransport->second == "Zmq" )
        {

        }
        else
        {
            if( m_DebugMode == true )
                throw std::invalid_argument( "ERROR: transport + " + itTransport->second + " not supported, in " +
                                              m_Name + m_EndMessage );
        }
    }
}


std::string DataManWriter::GetMdtmParameter( const std::string parameter, const std::map<std::string,std::string>& mdtmParameters )
{
    auto itParam = mdtmParameters.find( parameter );
    if( itParam != mdtmParameters.end() ) //found
    {
        return itParam->second; //return value
    }
    // if not found
    //mandatory ones
    if( parameter == "localIP" || parameter == "remoteIP" )
    {
        if( m_DebugMode == true )
            throw std::invalid_argument( "ERROR: " + parameter + " parameter not found in Method, in call to DataManWriter constructor\n" );
    }
    else if( parameter == "prefix" )
    {
        return "";
    }
    else if( parameter == "pipes" )
    {
        return "0"; // or 1?
    }
    else if( parameter == "tolerances" ) //so far empty string
    {

    }
    else if( parameter == "priority" )
    {

    }

    return ""; //return empty string
}


} //end namespace adios
