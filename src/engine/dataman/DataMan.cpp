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

//supported capsules
#include "capsule/Heap.h"

//supported transports
#include "transport/POSIX.h"
#include "transport/FStream.h"
#include "transport/File.h"


namespace adios
{
namespace engine
{

DataMan::DataMan( const std::string streamName, const std::string accessMode, const MPI_Comm mpiComm,
                  const Method& method, const bool debugMode, const unsigned int cores ):
    Engine( "DataMan", streamName, accessMode, mpiComm, method, debugMode, cores, " DataMan constructor (or call to ADIOS Open).\n" )
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
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}


void DataMan::Write( Group& group, const std::string variableName, const unsigned char* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned char*" );
	Variable<unsigned char>& variable = group.m_UChar[index]; //must be a reference
    variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}


void DataMan::Write( Group& group, const std::string variableName, const short* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write short*" );
	Variable<short>& variable = group.m_Short[index]; //must be a reference
    variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}


void DataMan::Write( Group& group, const std::string variableName, const unsigned short* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned short*" );
	Variable<unsigned short>& variable = group.m_UShort[index]; //must be a reference
    variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}


void DataMan::Write( Group& group, const std::string variableName, const int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write int*" );
	Variable<int>& variable = group.m_Int[index]; //must be a reference
    variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}


void DataMan::Write( Group& group, const std::string variableName, const unsigned int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned int*" );
	Variable<unsigned int>& variable = group.m_UInt[index]; //must be a reference
    variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}


void DataMan::Write( Group& group, const std::string variableName, const long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write long int*" );
	Variable<long int>& variable = group.m_LInt[index]; //must be a reference
    variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}


void DataMan::Write( Group& group, const std::string variableName, const unsigned long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned long int*" );
	Variable<unsigned long int>& variable = group.m_ULInt[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}


void DataMan::Write( Group& group, const std::string variableName, const long long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write long long int*" );
	Variable<long long int>& variable = group.m_LLInt[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}


void DataMan::Write( Group& group, const std::string variableName, const unsigned long long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned long long int*" );
	Variable<unsigned long long int>& variable = group.m_ULLInt[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}

void DataMan::Write( Group& group, const std::string variableName, const float* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write float*" );
	Variable<float>& variable = group.m_Float[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}

void DataMan::Write( Group& group, const std::string variableName, const double* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write double*" );
	Variable<double>& variable = group.m_Double[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
}


void DataMan::Write( Group& group, const std::string variableName, const long double* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write long double*" );
	Variable<long double>& variable = group.m_LDouble[index]; //must be a reference
	variable.Values = values;
	DataManWriteVariable( group, variableName, variable, m_Capsules, m_Transports );
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


void DataMan::InitCapsules( )
{
    //Create single capsule of type heap
    m_Capsules.push_back( std::make_shared<Heap>( m_AccessMode, m_RankMPI, m_Cores ) );
}


void DataMan::InitTransports( ) //maybe move this?
{
    std::set< std::string > transportStreamNames; //used to check for name conflict between transports

    const unsigned int transportsSize = m_Method.m_TransportParameters.size();

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
        else if( itTransport->second == "MPIFile" )
        {
            //m_Transports.push_back( std::make_shared<MPIFile>( m_MPIComm, m_DebugMode ) ); not yet supported
        }
        else
        {
            if( m_DebugMode == true )
                throw std::invalid_argument( "ERROR: transport + " + itTransport->second + " not supported, in " +
                                              m_Name + m_EndMessage );
        }
        //name
        if( transportsSize > 1 )
        {
            auto itName = parameters.find( "name" ); //first check name

            if( m_DebugMode == true )
                CheckParameter( itName, parameters, "name", " in transport " + itTransport->second +
                                ", in " + m_Name + m_EndMessage );

            m_Transports.back()->Open( itName->second, m_AccessMode );
        }
        else if( transportsSize == 1 )
        {
            auto itName = parameters.find( "name" );

            if( itName == parameters.end() ) //take streamName
                m_Transports.back()->Open( m_Name, m_AccessMode );
            else
                m_Transports.back()->Open( m_Name, m_AccessMode );

        }
        else if( transportsSize == 0 )
        {
            if( m_DebugMode == true )
                throw std::invalid_argument( "ERROR: transport not defined for engine " + m_Name + m_EndMessage );
        }
    }
}


} //end namespace engine
} //end namespace adios



