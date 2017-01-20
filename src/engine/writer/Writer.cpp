/*
 * SingleBP.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */

#include <iostream>

#include "engine/writer/Writer.h"

#include "../../../include/engine/writer/WriterTemplates.h"
#include "core/Support.h"
#include "functions/adiosFunctions.h" //GetTotalSize

//supported capsules
#include "capsule/Heap.h"

//supported transports
#include "transport/POSIX.h"
#include "transport/FStream.h"
#include "transport/File.h"


namespace adios
{


Writer::Writer( const std::string streamName, const std::string accessMode, const MPI_Comm mpiComm,
                const Method& method, const bool debugMode, const unsigned int cores ):
    Engine( "Writer", streamName, accessMode, mpiComm, method, debugMode, cores, " Writer constructor (or call to ADIOS Open).\n" )
{
    Init( );
}


Writer::~Writer( )
{ }


void Writer::Init( )
{
    InitCapsules( );
    InitTransports( );
}


void Writer::Write( Group& group, const std::string variableName, const char* values )
{

}

void Writer::Write( Group& group, const std::string variableName, const unsigned char* values )
{

}

void Writer::Write( Group& group, const std::string variableName, const short* values )
{

}

void Writer::Write( Group& group, const std::string variableName, const unsigned short* values )
{

}

void Writer::Write( Group& group, const std::string variableName, const int* values )
{

}

void Writer::Write( Group& group, const std::string variableName, const unsigned int* values )
{

}

void Writer::Write( Group& group, const std::string variableName, const long int* values )
{

}

void Writer::Write( Group& group, const std::string variableName, const unsigned long int* values )
{

}

void Writer::Write( Group& group, const std::string variableName, const long long int* values )
{

}

void Writer::Write( Group& group, const std::string variableName, const unsigned long long int* values )
{

}

void Writer::Write( Group& group, const std::string variableName, const float* values )
{

}


void Writer::Write( Group& group, const std::string variableName, const double* values )
{
    //auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("double"), " from call to Write double*" );
}


void Writer::Write( Group& group, const std::string variableName, const long double* values )
{

}


void Writer::Write( const std::string variableName, const char* values )
{

}

void Writer::Write( const std::string variableName, const unsigned char* values )
{

}

void Writer::Write( const std::string variableName, const short* values )
{

}

void Writer::Write( const std::string variableName, const unsigned short* values )
{

}

void Writer::Write( const std::string variableName, const int* values )
{
    const unsigned int index = PreSetVariable( *m_Group, variableName, Support::DatatypesAliases.at("int"), " from call to Write int*" );
    m_Group->m_Int[index].Values = values;
    //here call the Template function WriteHelper( m_Group, variable, )


}

void Writer::Write( const std::string variableName, const unsigned int* values )
{

}

void Writer::Write( const std::string variableName, const long int* values )
{

}

void Writer::Write( const std::string variableName, const unsigned long int* values )
{

}

void Writer::Write( const std::string variableName, const long long int* values )
{

}

void Writer::Write( const std::string variableName, const unsigned long long int* values )
{

}

void Writer::Write( const std::string variableName, const float* values )
{

}

void Writer::Write( const std::string variableName, const double* values )
{
    auto index = PreSetVariable( *m_Group, variableName, Support::DatatypesAliases.at("double"), " from call to Write double*" );
    std::cout << "Hello from SingleBP Write double with index " << index << "\n";
}


void Writer::Write( const std::string variableName, const long double* values )
{

}


void Writer::InitCapsules( )
{
    if( m_DebugMode == true )
    {
        if( m_Method.m_CapsuleParameters.size() > 1 )
        {
            throw std::invalid_argument( "ERROR: SingleBP engine only allows one heap buffer, in " + m_Name +
                                         m_EndMessage );
        }
        else if( m_Method.m_CapsuleParameters.size() == 1 )
        {
            auto itType = m_Method.m_CapsuleParameters[0].find( "buffer" );

            if( m_DebugMode == true )
                CheckParameter( itType, m_Method.m_CapsuleParameters[0], " capsule buffer",
                                ", in " + m_Name + m_EndMessage );

            if( !( itType->second == "Heap" || itType->second == "HEAP" ) )
                throw std::invalid_argument( "ERROR: SingleBP doesn't support Capsule of buffer type " +
                                              itType->second + " in " + m_Name + m_EndMessage );
        }
    }
    //Create single capsule of type heap
    m_Capsules.push_back( std::make_shared<Heap>( m_AccessMode, m_RankMPI, m_Cores ) );
}



void Writer::InitTransports( )
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
            //m_Transports.push_back( std::make_shared<MPIFile>( m_MPIComm, m_DebugMode ) );
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



} //end namespace adios

