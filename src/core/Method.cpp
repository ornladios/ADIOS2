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


Method::Method( const std::string type, Group& group, const bool debugMode ):
    m_Type{ type },
    m_DebugMode{ debugMode },
    m_Group{ &group }
{ }


Method::~Method( )
{ }


//PRIVATE Functions
void Method::AddTransportParameters( const std::string type, const std::vector<std::string>& parameters )
{
    if( m_DebugMode == true )
    {
        if( type.empty() || type.find("=") != type.npos )
            throw std::invalid_argument( "ERROR: first argument in AddTransport must be a single word for transport\n" );
    }

    std::map<std::string, std::string> mapParameters = BuildParametersMap( parameters, m_DebugMode );
    if( m_DebugMode == true )
    {
        if( mapParameters.count("transport") == 1 )
            std::invalid_argument( "ERROR: transport can't be redefined with \"transport=type\", "
                                   "type must be the first argument\n" );
    }

    mapParameters["transport"] = type;
    m_TransportParameters.push_back( mapParameters );
}

void Method::SetDefaultGroup( Group& group )
{
    m_Group = &group;
}




} //end namespace


