/*
 * DataMan.cpp
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#include <iostream>


#include "engine/dataman/DataMan.h"
#include "engine/dataman/DataManTemplates.h"
#include "core/Support.h"
#include "functions/adiosFunctions.h" //CSVToVector

//supported capsules
#include "capsule/Heap.h"

//supported transports
#include "transport/POSIX.h"
#include "transport/FStream.h"
#include "transport/File.h"
#include "transport/MdtmMan.h"


namespace adios
{
namespace engine
{

DataMan::DataMan( const std::string streamName, const std::string accessMode, const MPI_Comm mpiComm,
                  const Method& method, const bool debugMode, const unsigned int cores ):
    Engine( "DataMan", streamName, accessMode, mpiComm, method, debugMode, cores, " DataMan constructor (or call to ADIOS Open).\n" ),
    m_Buffer{ Heap( accessMode, m_RankMPI, m_DebugMode, cores ) }
{
    Init( );
}


DataMan::~DataMan( )
{ }


void DataMan::Init( )
{
    InitCapsules( );
    InitTransports( );
}


void DataMan::Write( Group& group, const std::string variableName, const char* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write char*" );
	Variable<char>& variable = group.m_Char[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}


void DataMan::Write( Group& group, const std::string variableName, const unsigned char* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned char*" );
	Variable<unsigned char>& variable = group.m_UChar[index]; //must be a reference
    variable.Values = values;
    DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}


void DataMan::Write( Group& group, const std::string variableName, const short* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write short*" );
	Variable<short>& variable = group.m_Short[index]; //must be a reference
    variable.Values = values;
    DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}


void DataMan::Write( Group& group, const std::string variableName, const unsigned short* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned short*" );
	Variable<unsigned short>& variable = group.m_UShort[index]; //must be a reference
    variable.Values = values;
    DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}


void DataMan::Write( Group& group, const std::string variableName, const int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write int*" );
	Variable<int>& variable = group.m_Int[index]; //must be a reference
    variable.Values = values;
    DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}


void DataMan::Write( Group& group, const std::string variableName, const unsigned int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned int*" );
	Variable<unsigned int>& variable = group.m_UInt[index]; //must be a reference
    variable.Values = values;
    DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}


void DataMan::Write( Group& group, const std::string variableName, const long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write long int*" );
	Variable<long int>& variable = group.m_LInt[index]; //must be a reference
    variable.Values = values;
    DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}


void DataMan::Write( Group& group, const std::string variableName, const unsigned long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned long int*" );
	Variable<unsigned long int>& variable = group.m_ULInt[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}


void DataMan::Write( Group& group, const std::string variableName, const long long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write long long int*" );
	Variable<long long int>& variable = group.m_LLInt[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}


void DataMan::Write( Group& group, const std::string variableName, const unsigned long long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned long long int*" );
	Variable<unsigned long long int>& variable = group.m_ULLInt[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}


void DataMan::Write( Group& group, const std::string variableName, const float* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write float*" );
	Variable<float>& variable = group.m_Float[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}

void DataMan::Write( Group& group, const std::string variableName, const double* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write double*" );
	Variable<double>& variable = group.m_Double[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}


void DataMan::Write( Group& group, const std::string variableName, const long double* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write long double*" );
	Variable<long double>& variable = group.m_LDouble[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Buffer, m_Transports, m_BP1Writer );
}

//USING Preset Group
void DataMan::Write( const std::string variableName, const char* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const unsigned char* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const short* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const unsigned short* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const unsigned int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const long int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const unsigned long int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const long long int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const unsigned long long int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const float* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const double* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void DataMan::Write( const std::string variableName, const long double* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}


void DataMan::InitTransports( ) //maybe move this?
{
    std::set< std::string > transportStreamNames; //used to check for name conflict between transports

    //const unsigned int transportsSize = m_Method.m_TransportParameters.size();

    for( const auto& parameters : m_Method.m_TransportParameters )
    {
        auto itTransport = parameters.find( "transport" );
        if( m_DebugMode == true )
            CheckParameter( itTransport, parameters, "transport", ", in " + m_Name + m_EndMessage );

        if( itTransport->second == "POSIX" )
        {
            m_Transports.push_back( std::make_shared<POSIX>( m_MPIComm, m_DebugMode ) );
        }
        else if( itTransport->second == "File" )
        {
            m_Transports.push_back( std::make_shared<File>( m_MPIComm, m_DebugMode ) );
        }
        else if( itTransport->second == "FStream" )
        {
            m_Transports.push_back( std::make_shared<FStream>( m_MPIComm, m_DebugMode ) );
        }
        else if( itTransport->second == "Mdtm" || itTransport->second == "MdtmMan" )
        {
            const std::string localIP( GetMdtmParameter( "localIP", parameters ) ); //mandatory
            const std::string remoteIP( GetMdtmParameter( "remoteIP", parameters ) ); //mandatory
            const std::string prefix( GetMdtmParameter( "prefix", parameters ) );
            const int numberOfPipes = std::stoi( GetMdtmParameter( "pipes", parameters ) );
            const std::vector<int> tolerances = CSVToVectorInt( GetMdtmParameter( "tolerances", parameters ) );
            const std::vector<int> priorities = CSVToVectorInt( GetMdtmParameter( "priorities", parameters ) );

            m_Transports.push_back( std::make_shared<MdtmMan>( localIP, remoteIP, m_AccessMode, prefix, numberOfPipes,
                                                               tolerances, priorities, m_MPIComm, m_DebugMode ) );
        }
        else if( itTransport->second == "MPIFile" )
        {
            //m_Transports.push_back( std::make_shared<MPIFile>( m_MPIComm, m_DebugMode ) ); //not yet supported
        }
        else
        {
            if( m_DebugMode == true )
                throw std::invalid_argument( "ERROR: transport + " + itTransport->second + " not supported, in " +
                                              m_Name + m_EndMessage );
        }
    }
}


std::string DataMan::GetMdtmParameter( const std::string parameter, const std::map<std::string,std::string>& mdtmParameters )
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
            throw std::invalid_argument( "ERROR: " + parameter + " parameter not found in Method, in call to DataMan constructor\n" );
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



} //end namespace engine
} //end namespace adios



