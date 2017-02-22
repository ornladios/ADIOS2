/*
 * ADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy
 */

/// \cond EXCLUDE_FROM_DOXYGEN
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
/// \endcond

#include "ADIOS.h"


#include "functions/adiosFunctions.h"

//Engines
#include "engine/bp/BPWriter.h"
#include "engine/dataman/DataManWriter.h"
#include "engine/dataman/DataManReader.h"


namespace adios
{


ADIOS::ADIOS( const bool debugMode ):
    m_DebugMode{ debugMode }
{
    InitMPI( );
}


ADIOS::ADIOS( const std::string configFileName, const bool debugMode ):
    m_ConfigFile{ configFileName },
    m_DebugMode{ debugMode }
{
   InitMPI( );
    // InitXML( m_ConfigFile, m_MPIComm, m_DebugMode, m_Transforms );
}


ADIOS::ADIOS( const std::string xmlConfigFile, const MPI_Comm mpiComm, const bool debugMode  ):
    m_MPIComm{ mpiComm },
    m_ConfigFile{ xmlConfigFile },
	m_DebugMode{ debugMode }
{
    InitMPI( );
    //InitXML( m_XMLConfigFile, m_MPIComm, m_DebugMode, m_HostLanguage, m_Transforms, m_Groups );
}


ADIOS::ADIOS( const MPI_Comm mpiComm, const bool debugMode ):
    m_MPIComm{ mpiComm },
    m_DebugMode{ debugMode }
{
    InitMPI( );
}


ADIOS::~ADIOS( )
{ }


void ADIOS::InitMPI( )
{
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
}


Method& ADIOS::DeclareMethod( const std::string methodName, const std::string type )
{
    if( m_DebugMode == true )
    {
        if( m_Methods.count( methodName ) == 1 )
            throw std::invalid_argument( "ERROR: method " + methodName + " already declared, from DeclareMethod\n" );
    }
    m_Methods.emplace( methodName, Method( type, m_DebugMode ) );
    return m_Methods.at( methodName );
}


std::shared_ptr<Engine> ADIOS::Open( const std::string name, const std::string accessMode, MPI_Comm mpiComm, const Method& method, const unsigned int cores )
{
    if( m_DebugMode == true )
    {
        if( m_EngineNames.count( name ) == 1 ) //Check if Engine already exists
            throw std::invalid_argument( "ERROR: engine name " + name + " already created by Open, in call from Open.\n" );
    }

    m_EngineNames.insert( name );

    const std::string type( method.m_Type );

    if( type == "BPWriter" || type == "bpwriter" )
    {
        return std::make_shared<BPWriter>( *this, name, accessMode, mpiComm, method, m_DebugMode, cores );
    }
    else if( type == "SIRIUS" || type == "sirius" || type == "Sirius" )
    {
        //not yet supported
        //return std::make_shared<engine::DataMan>( name, accessMode, mpiComm, method, m_DebugMode, cores, m_HostLanguage );
    }
    else if( type == "DataManWriter" )
    {
    	return std::make_shared<DataManWriter>( *this, name, accessMode, mpiComm, method, m_DebugMode, cores );
    }
    else if( type == "DataManReader" )
    {
        return std::make_shared<DataManReader>( *this, name, accessMode, mpiComm, method, m_DebugMode, cores );
    }
    else if( type == "Vis" )
    {
        //return std::make_shared<Vis>( *this, name, accessMode, mpiComm, method, m_DebugMode, cores );
    }
    else
    {
        if( m_DebugMode == true )
            throw std::invalid_argument( "ERROR: method type " + type + " not supported for " + name + ", in call to Open\n" );
    }

    return nullptr; // if debug mode is off
}


std::shared_ptr<Engine> ADIOS::Open( const std::string streamName, const std::string accessMode, const Method& method,
                                     const unsigned int cores )
{
    return Open( streamName, accessMode, m_MPIComm, method, cores );
}


std::shared_ptr<Engine> ADIOS::Open( const std::string name, const std::string accessMode, MPI_Comm mpiComm,
                                     const std::string methodName, const unsigned int cores )
{
    auto itMethod = m_Methods.find( methodName );

    if( m_DebugMode == true )
    {
        CheckMethod( itMethod, methodName, " in call to Open\n" );
    }

    return Open( name, accessMode, mpiComm, itMethod->second, cores );
}


std::shared_ptr<Engine> ADIOS::Open( const std::string name, const std::string accessMode,
                                     const std::string methodName, const unsigned int cores )
{
    return Open( name, accessMode, m_MPIComm, methodName, cores );
}


VariableCompound& ADIOS::GetVariableCompound( const std::string name )
{
    return m_Compound[ GetVariableIndex<void>(name) ];
}


void ADIOS::MonitorVariables( std::ostream& logStream )
{
    logStream << "\tVariable \t Type\n";

    for( auto& variablePair : m_Variables )
    {
        const std::string name( variablePair.first );
        const std::string type( variablePair.second.first );

        if( type == GetType<char>() )
            GetVariable<char>( name ).Monitor( logStream );

        else if( type == GetType<unsigned char>() )
            GetVariable<unsigned char>( name ).Monitor( logStream );

        else if( type == GetType<short>() )
            GetVariable<short>( name ).Monitor( logStream );

        else if( type == GetType<unsigned short>() )
            GetVariable<unsigned short>( name ).Monitor( logStream );

        else if( type == GetType<int>() )
            GetVariable<int>( name ).Monitor( logStream );

        else if( type == GetType<unsigned int>() )
            GetVariable<unsigned int>( name ).Monitor( logStream );

        else if( type == GetType<long int>() )
            GetVariable<long int>( name ).Monitor( logStream );

        else if( type == GetType<unsigned long int>() )
            GetVariable<unsigned long int>( name ).Monitor( logStream );

        else if( type == GetType<long long int>() )
            GetVariable<long long int>( name ).Monitor( logStream );

        else if( type == GetType<unsigned long long int>() )
            GetVariable<unsigned long long int>( name ).Monitor( logStream );

        else if( type == GetType<float>() )
            GetVariable<float>( name ).Monitor( logStream );

        else if( type == GetType<double>() )
            GetVariable<double>( name ).Monitor( logStream );

        else if( type == GetType<long double>() )
            GetVariable<long double>( name ).Monitor( logStream );
    }
}


//PRIVATE FUNCTIONS BELOW
void ADIOS::CheckVariableInput( const std::string name, const Dims& dimensions ) const
{
    if( m_DebugMode == true )
    {
        if( m_Variables.count( name ) == 1 )
            throw std::invalid_argument( "ERROR: variable " + name + " already exists, in call to DefineVariable\n" );

        if( dimensions.empty() == true )
            throw std::invalid_argument( "ERROR: variable " + name + " dimensions can't be empty, in call to DefineVariable\n" );
    }
}


void ADIOS::CheckVariableName( std::map< std::string, std::pair< std::string, unsigned int > >::const_iterator itVariable,
                               const std::string name, const std::string hint ) const
{
    if( m_DebugMode == true )
    {
        if( itVariable == m_Variables.end() )
            throw std::invalid_argument( "ERROR: variable " + name + " does not exist " + hint + "\n" );
    }
}


void ADIOS::CheckMethod( std::map< std::string, Method >::const_iterator itMethod,
                         const std::string methodName, const std::string hint ) const
{
    if( itMethod == m_Methods.end() )
        throw std::invalid_argument( "ERROR: method " + methodName + " not found " + hint + "\n" );
}


} //end namespace
