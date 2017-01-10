/*
 * Method.cpp
 *
 *  Created on: Jan 6, 2017
 *      Author: wfg
 */


#include "core/Method.h"
#include "functions/adiosFunctions.h"


namespace adios
{


Method::Method( const std::string type, const bool debugMode ):
    m_Type{ type },
    m_DebugMode{ debugMode }
{ }

Method::~Method( )
{ }


//PRIVATE Functions
void Method::AddCapsuleParameters( const std::string type, const std::vector<std::string>& parameters )
{
    if( m_DebugMode == true )
    {
        if( type.empty() || type.find("=") != type.npos )
            throw std::invalid_argument( "ERROR: first argument in AddCapsule must be a single word for capsule (buffer) type\n" );
    }

    std::map<std::string, std::string> mapParameters = BuildParametersMap(parameters, m_DebugMode);
    if( m_DebugMode == true )
    {
        if( mapParameters.count("buffer") )
            throw std::invalid_argument( "ERROR: buffer can't be redefined with buffer=, "
                                         "must be the first argument, in AddCapsuleParameters( bufferType, ...);\n" );
    }
    mapParameters["buffer"] = type;
    m_CapsuleParameters.push_back( std::move( mapParameters ) );
}


void Method::AddTransportParameters( const std::string type, const std::vector<std::string>& parameters )
{
    if( m_DebugMode == true )
    {
        if( type.empty() || type.find("=") != type.npos )
            throw std::invalid_argument( "ERROR: first argument in AddTransport must be a single word for transport\n" );
    }

    std::map<std::string, std::string> mapParameters = BuildParametersMap(parameters, m_DebugMode);
    if( m_DebugMode == true )
    {
        if( mapParameters.count("transport") )
            std::invalid_argument( "ERROR: transport can't be redefined with transport=, "
                                   "must be the first argument, in AddTransportParameters( transport, ...);\n" );
    }

    mapParameters["transport"] = type;
    m_TransportParameters.push_back( BuildParametersMap(parameters, m_DebugMode) );
}


} //end namespace


