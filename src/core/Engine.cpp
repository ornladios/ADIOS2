/*
 * Engine.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */


#include "core/Engine.h"
#include "functions/engineTemplates.h"


namespace adios
{


Engine::Engine( const std::string engineType, const std::string name, const std::string accessMode,
                const MPI_Comm mpiComm, const Method& method,
                const bool debugMode ):
    m_EngineType{ engineType },
    m_Name{ name },
    m_AccessMode{ accessMode },
    m_Method{ &method },
    m_MPIComm{ mpiComm }
{
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
}


Engine::~Engine( )
{ }


//WRITE Functions
void Engine::Write( Group& group, const std::string variableName, const char* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("char"), " from call to Write char*" );
    WriteToCapsules( group, group.m_Char[index], values, m_Capsules, m_Transports );
}


void Engine::Write( Group& group, const std::string variableName, const unsigned char* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("unsigned char"), " from call to Write unsigned char*" );
    WriteToCapsules( group, group.m_UChar[index], values, m_Capsules, m_Transports );
}


void Engine::Write( Group& group, const std::string variableName, const short* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("short"), " from call to Write short*" );
    WriteToCapsules( group, group.m_Short[index], values, m_Capsules, m_Transports );
}


void Engine::Write( Group& group, const std::string variableName, const unsigned short* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("unsigned short"), " from call to Write unsigned short*" );
    WriteToCapsules( group, group.m_UShort[index], values, m_Capsules, m_Transports );
}


void Engine::Write( Group& group, const std::string variableName, const int* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("int"), " from call to Write int*" );
    WriteToCapsules( group, group.m_Int[index], values, m_Capsules, m_Transports );
}


void Engine::Write( Group& group, const std::string variableName, const unsigned int* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("unsigned int"), " from call to Write unsigned int*" );
    WriteToCapsules( group, group.m_UInt[index], values, m_Capsules, m_Transports );
}


void Engine::Write( Group& group, const std::string variableName, const long int* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("long int"), " from call to Write long int*" );
    WriteToCapsules( group, group.m_LInt[index], values, m_Capsules, m_Transports );
}


void Engine::Write( Group& group, const std::string variableName, const unsigned long int* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("unsigned long int"), " from call to Write unsigned long int*" );
    WriteToCapsules( group, group.m_ULInt[index], values, m_Capsules, m_Transports );
}


void Engine::Write( Group& group, const std::string variableName, const long long int* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("long long int"), " from call to Write long long int*" );
    WriteToCapsules( group, group.m_LLInt[index], values, m_Capsules, m_Transports );
}


void Engine::Write( Group& group, const std::string variableName, const unsigned long long int* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("unsigned long long int"), " from call to Write unsigned long long int*" );
    WriteToCapsules( group, group.m_ULLInt[index], values, m_Capsules, m_Transports );
}


void Engine::Write( Group& group, const std::string variableName, const float* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("float"), " from call to Write float*" );
    WriteToCapsules( group, group.m_Float[index], values, m_Capsules, m_Transports );
}


void Engine::Write( Group& group, const std::string variableName, const double* values )
{
    auto index = PreSetVariable( group, variableName, Support::DatatypesAliases.at("double"), " from call to Write double*" );
    WriteToCapsules( group, group.m_Double[index], values, m_Capsules, m_Transports );
}


const unsigned int Engine::PreSetVariable( Group& group, const std::string variableName,
                                           const std::set<std::string>& types,
                                           const std::string hint ) const
{
    auto itVariable = group.m_Variables.find( variableName );

    if( m_DebugMode == true )
    {
        if( itVariable == group.m_Variables.end() )
            throw std::invalid_argument( "ERROR: variable " + variableName + " doesn't exist " + hint + ".\n" );

        if( IsTypeAlias( itVariable->first, types ) == false )
                throw std::invalid_argument( "ERROR: type in variable " + variableName + " doesn't match " + hint + ".\n" );
    }

    group.m_WrittenVariables.insert( variableName ); //should be done before writing to buffer, in case there is a crash?
    unsigned int index = itVariable->second.second;
    return index;
}


void Engine::Close( int transportIndex )
{
    if( transportIndex == -1 ) //close all transports
    {
        for( auto& transport : m_Transports )
            transport->Close( );
    }
    else
    {
        m_Transports[transportIndex]->Close( ); //here need to pass the buffer
    }
}


//PROTECTED FUNCTIONS
void Engine::SetTransports( )
{
    for( const auto& transportPair : m_Method->Transports )
    {
        const std::string transport = transportPair.first;
        const std::vector<std::string>& arguments = transportPair.second;

        if( transport == "POSIX" )
            m_Transports.push_back( std::make_shared<POSIX>( m_MPIComm, m_DebugMode, arguments ) );

        else if( transport == "FStream" )
            m_Transports.push_back( std::make_shared<FStream>( m_MPIComm, m_DebugMode, arguments ) );

        else
        {
            if( m_DebugMode == true )
                throw std::invalid_argument( "ERROR: transport + " + transport + " not supported, in Engine constructor (or Open).\n" );
        }

        std::string name = GetName( arguments );
        m_Transports.back()->Open( name, m_AccessMode );
    }
}



std::string Engine::GetName( const std::vector<std::string>& arguments ) const
{
    bool isNameFound = false;
    std::string name;

    for( const auto argument : arguments )
    {
        auto namePosition = argument.find( "name=" );
        if( namePosition != argument.npos )
        {
            isNameFound = true;
            name = argument.substr( namePosition + 5 );
            break;
        }
    }

    if( m_DebugMode == true )
    {
        if( name.empty() || isNameFound == false )
            std::invalid_argument( "ERROR: argument to name= is empty or not found in call to AddTransport" );
    }

    return name;
}


} //end namespace



