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
void Method::AddCapsuleParameters( const std::vector<std::string>& parameters )
{
    m_CapsuleParameters.push_back( BuildParametersMap(parameters, m_DebugMode) );
}


void Method::AddTransportParameters( const std::vector<std::string>& parameters )
{
    m_TransportParameters.push_back( BuildParametersMap(parameters, m_DebugMode) );
}


} //end namespace


