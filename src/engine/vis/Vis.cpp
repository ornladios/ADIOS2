/*
 * Vis.cpp
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#include <iostream>


#include "engine/vis/Vis.h"
#include "engine/vis/VisTemplates.h"
#include "core/Support.h"
#include "functions/adiosFunctions.h" //CSVToVector

//supported capsules
#include "capsule/Heap.h"
#include "capsule/ShmSystemV.h"

//supported transports
#include "transport/POSIX.h"
#include "transport/FStream.h"
#include "transport/File.h"


namespace adios
{
namespace engine
{

Vis::Vis( const std::string streamName, const std::string accessMode, const MPI_Comm mpiComm,
          const Method& method, const bool debugMode, const unsigned int cores,
          const std::string hostLanguage ):
    Engine( "Vis", streamName, accessMode, mpiComm, method, debugMode, cores,
            " Vis constructor (or call to ADIOS Open).\n", hostLanguage )
{
    Init( );
}


Vis::~Vis( )
{ }


void Vis::Init( )
{
    InitTransports( );
}


void Vis::Write( Group& group, const std::string variableName, const char* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write char*" );
	Variable<char>& variable = group.m_Char[index]; //must be a reference
	variable.Values = values;
	VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}


void Vis::Write( Group& group, const std::string variableName, const unsigned char* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned char*" );
	Variable<unsigned char>& variable = group.m_UChar[index]; //must be a reference
    variable.Values = values;
    VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}


void Vis::Write( Group& group, const std::string variableName, const short* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write short*" );
	Variable<short>& variable = group.m_Short[index]; //must be a reference
    variable.Values = values;
    VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}


void Vis::Write( Group& group, const std::string variableName, const unsigned short* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned short*" );
	Variable<unsigned short>& variable = group.m_UShort[index]; //must be a reference
    variable.Values = values;
    VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}


void Vis::Write( Group& group, const std::string variableName, const int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write int*" );
	Variable<int>& variable = group.m_Int[index]; //must be a reference
    variable.Values = values;
    VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}


void Vis::Write( Group& group, const std::string variableName, const unsigned int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned int*" );
	Variable<unsigned int>& variable = group.m_UInt[index]; //must be a reference
    variable.Values = values;
    VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}


void Vis::Write( Group& group, const std::string variableName, const long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write long int*" );
	Variable<long int>& variable = group.m_LInt[index]; //must be a reference
    variable.Values = values;
    VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}


void Vis::Write( Group& group, const std::string variableName, const unsigned long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned long int*" );
	Variable<unsigned long int>& variable = group.m_ULInt[index]; //must be a reference
	variable.Values = values;
	VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}


void Vis::Write( Group& group, const std::string variableName, const long long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write long long int*" );
	Variable<long long int>& variable = group.m_LLInt[index]; //must be a reference
	variable.Values = values;
	VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}


void Vis::Write( Group& group, const std::string variableName, const unsigned long long int* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write unsigned long long int*" );
	Variable<unsigned long long int>& variable = group.m_ULLInt[index]; //must be a reference
	variable.Values = values;
	VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}


void Vis::Write( Group& group, const std::string variableName, const float* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write float*" );
	Variable<float>& variable = group.m_Float[index]; //must be a reference
	variable.Values = values;
	VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}

void Vis::Write( Group& group, const std::string variableName, const double* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write double*" );
	Variable<double>& variable = group.m_Double[index]; //must be a reference
	variable.Values = values;
	VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}


void Vis::Write( Group& group, const std::string variableName, const long double* values )
{
	auto index = PreSetVariable( group, variableName, " from call to Write long double*" );
	Variable<long double>& variable = group.m_LDouble[index]; //must be a reference
	variable.Values = values;
	VisWriteVariable( group, variableName, variable, m_Capsules, m_Transports, m_BP1Writer, m_RankMPI );
}

//USING Preset Group
void Vis::Write( const std::string variableName, const char* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const unsigned char* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const short* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const unsigned short* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const unsigned int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const long int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const unsigned long int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const long long int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const unsigned long long int* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const float* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const double* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}

void Vis::Write( const std::string variableName, const long double* values )
{
	CheckDefaultGroup( );
	Write( *m_Group, variableName, values );
}


void Vis::InitTransports( ) //maybe move this?
{
    TransportNamesUniqueness( );

    for( const auto& parameters : m_Method.m_TransportParameters )
    {
        auto itTransport = parameters.find( "transport" );

        if( itTransport->second == "POSIX" )
        {
            m_Transports.push_back( std::make_shared<POSIX>( m_MPIComm, m_DebugMode ) );
            m_Transports.back()->Open( m_Name, m_AccessMode );
        }
        else if( itTransport->second == "ShMem" )
        {
            //m_Transports.push_back( std::make_shared<ShMemTransport>( m_MPIComm, m_DebugMode ) );
        }
        else if( itTransport->second == "VisIt" )
        {
            //m_Transports.push_back( std::make_shared<SomeStagingTransport>( m_MPIComm, m_DebugMode ) ); //not yet supported
            //
        }
        else
        {
            if( m_DebugMode == true )
                throw std::invalid_argument( "ERROR: transport " + itTransport->second + " not supported, in " +
                                              m_Name + m_EndMessage );
        }
    }
}



void Vis::Close( const int transportIndex )
{
     //do some preliminary work here
    // (e.g. process metadata )
    //flush the last piece of data or do more writes

    if( transportIndex == -1 ) // all transports
    {
        for( auto& transport : m_Transports )
            transport->Close( );
    }
    else
    {
        if( m_DebugMode == true )
        {
            if( transportIndex >= static_cast<int>( m_Transports.size() ) )
                throw std::invalid_argument( "ERROR: transportIndex " + std::to_string( transportIndex ) + " is out of range\n" );
        }

        m_Transports[ transportIndex ]->Close( );
    }

    std::cout << "I own many Capsules (buffers: heap, shared memory, RDMA ) and \n"
                 " many Transports ( POSIX, in-situ vis, staging, shared memory, RDMA )\n"
                 " and I can do whatever I need to do with them\n";
}



} //end namespace engine
} //end namespace adios



