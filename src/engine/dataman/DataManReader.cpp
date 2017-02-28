/*
 * DataManReader.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: wfg
 */


#include "engine/dataman/DataManReader.h"

#include "core/Support.h"
#include "functions/adiosFunctions.h" //CSVToVector

//supported transports
#include "transport/file/FD.h" // uses POSIX
#include "transport/file/FP.h" // uses C FILE*
#include "transport/file/FStream.h" // uses C++ fstream
#include "transport/wan/MdtmMan.h" //uses Mdtm library

#include "DataMan.h"  //here comes your DataMan header from external dataman library

namespace adios
{

DataManReader::DataManReader( ADIOS& adios, const std::string name, const std::string accessMode, MPI_Comm mpiComm,
                              const Method& method, const bool debugMode, const unsigned int cores ):
    Engine( adios, "DataManReader", name, accessMode, mpiComm, method, debugMode, cores, " DataManReader constructor (or call to ADIOS Open).\n" ),
    m_Buffer( accessMode, m_RankMPI, m_DebugMode )
{
    Init( );
}

DataManReader::~DataManReader( )
{ }

void DataManReader::SetCallBack( std::function<void( const void*, std::string, std::string, std::string, Dims )> callback )
{
    m_CallBack = callback;
    m_Man.reg_callback(callback);
}

Variable<void>* DataManReader::InquireVariable( const std::string name, const bool readIn ) //not yet implemented
{ return nullptr; }

Variable<char>* DataManReader::InquireVariableChar( const std::string name, const bool readIn )
{ return InquireVariableCommon<char>( name, readIn ); }

Variable<unsigned char>* DataManReader::InquireVariableUChar( const std::string name, const bool readIn )
{ return InquireVariableCommon<unsigned char>( name, readIn ); }

Variable<short>* DataManReader::InquireVariableShort( const std::string name, const bool readIn )
{ return InquireVariableCommon<short>( name, readIn ); }

Variable<unsigned short>* DataManReader::InquireVariableUShort( const std::string name, const bool readIn )
{ return InquireVariableCommon<unsigned short>( name, readIn ); }

Variable<int>* DataManReader::InquireVariableInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<int>( name, readIn ); }

Variable<unsigned int>* DataManReader::InquireVariableUInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<unsigned int>( name, readIn ); }

Variable<long int>* DataManReader::InquireVariableLInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<long int>( name, readIn ); }

Variable<unsigned long int>* DataManReader::InquireVariableULInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<unsigned long int>( name, readIn ); }

Variable<long long int>* DataManReader::InquireVariableLLInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<long long int>( name, readIn ); }

Variable<unsigned long long int>* DataManReader::InquireVariableULLInt( const std::string name, const bool readIn )
{ return InquireVariableCommon<unsigned long long int>( name, readIn ); }

Variable<float>* DataManReader::InquireVariableFloat( const std::string name, const bool readIn )
{ return InquireVariableCommon<float>( name, readIn ); }

Variable<double>* DataManReader::InquireVariableDouble( const std::string name, const bool readIn )
{ return InquireVariableCommon<double>( name, readIn ); }

Variable<long double>* DataManReader::InquireVariableLDouble( const std::string name, const bool readIn )
{ return InquireVariableCommon<long double>( name, readIn ); }

Variable<std::complex<float>>* DataManReader::InquireVariableCFloat( const std::string name, const bool readIn )
{ return InquireVariableCommon<std::complex<float>>( name, readIn ); }

Variable<std::complex<double>>* DataManReader::InquireVariableCDouble( const std::string name, const bool readIn )
{ return InquireVariableCommon<std::complex<double>>( name, readIn ); }

Variable<std::complex<long double>>* DataManReader::InquireVariableCLDouble( const std::string name, const bool readIn )
{ return InquireVariableCommon<std::complex<long double>>( name, readIn ); }

VariableCompound* DataManReader::InquireVariableCompound( const std::string name, const bool readIn )
{ return nullptr; }


void DataManReader::Close( const int transportIndex )
{

}


//PRIVATE
void DataManReader::Init( )
{
    if( m_DebugMode == true )
    {
        if( m_AccessMode != "r" && m_AccessMode != "read" )
            throw std::invalid_argument( "ERROR: DataManReader doesn't support access mode " + m_AccessMode +
                                         ", in call to ADIOS Open or DataManReader constructor\n"  );
    }

    auto itRealTime = m_Method.m_Parameters.find( "real_time" );
    if( itRealTime != m_Method.m_Parameters.end() )
    {
        if( itRealTime->second == "yes" || itRealTime->second == "true" )
            m_DoRealTime = true;
    }

    if(m_DoRealTime)
    {
        /**
         * Lambda function that assigns a parameter in m_Method to a localVariable of type std::string
         */
        auto lf_AssignString = [this]( const std::string parameter, std::string& localVariable )
        {
            auto it = m_Method.m_Parameters.find( parameter );
            if( it != m_Method.m_Parameters.end() )
            {
                localVariable = it->second;
            }
        };

        /**
         * Lambda function that assigns a parameter in m_Method to a localVariable of type int
         */
        auto lf_AssignInt = [this]( const std::string parameter, int& localVariable )
        {
            auto it = m_Method.m_Parameters.find( parameter );
            if( it != m_Method.m_Parameters.end() )
            {
                localVariable = std::stoi( it->second );
            }
        };

        std::string method_type, method, local_ip, remote_ip; //no need to initialize to empty (it's default)
        int local_port=0, remote_port=0, num_channels=0;

        lf_AssignString( "method_type", method_type );
        if( method_type == "stream" )
        {
            lf_AssignString( "method", method );
            lf_AssignString( "local_ip", local_ip );
            lf_AssignString( "remote_ip", remote_ip );
            lf_AssignInt( "local_port", local_port );
            lf_AssignInt( "remote_port", remote_port );
            lf_AssignInt( "num_channels", num_channels );

            json jmsg;
            jmsg["method"] = method;
            jmsg["local_ip"] = local_ip;
            jmsg["remote_ip"] = remote_ip;
            jmsg["local_port"] = local_port;
            jmsg["remote_port"] = remote_port;
            jmsg["num_channels"] = num_channels;
            jmsg["stream_mode"] = "receiver";

            m_Man.add_stream(jmsg);
        }
    }
    else
    {
        InitCapsules( );
        InitTransports( );
    }
}


void DataManReader::InitCapsules( )
{
    //here init memory capsules
}


void DataManReader::InitTransports( ) //maybe move this?
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


std::string DataManReader::GetMdtmParameter( const std::string parameter, const std::map<std::string,std::string>& mdtmParameters )
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



}
