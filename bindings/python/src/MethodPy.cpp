/*
 * MethodPy.cpp
 *
 *  Created on: Mar 14, 2017
 *      Author: wfg
 */


#include "MethodPy.h"
#include "adiosPyFunctions.h"

namespace adios
{

MethodPy::MethodPy( const std::string type, const bool debugMode ):
    Method( type, debugMode )
{ }


MethodPy::~MethodPy( )
{ }


boost::python::object MethodPy::SetParametersPy( boost::python::tuple args, boost::python::dict kwargs )
{
    if( boost::python::len( args ) > 1  )
        throw std::invalid_argument( "ERROR: syntax of Method SetParameters function is incorrect, only use dictionary\n" );

    MethodPy& self = boost::python::extract<MethodPy&>( args[0] );
    self.m_Parameters = DictToMap( kwargs );
    return args[0];
}


boost::python::object MethodPy::AddTransportPy( boost::python::tuple args, boost::python::dict kwargs )
{
    if( boost::python::len( args ) != 2  )
        throw std::invalid_argument( "ERROR: syntax of Method AddTransport function is incorrect, only use one string for transport followed by a dictionary for parameters\n" );

    MethodPy& self = boost::python::extract<MethodPy&>( args[0] );
    std::string type = boost::python::extract<std::string>( args[1] );

    auto parameters = DictToMap( kwargs );
    parameters.insert( std::make_pair( "transport", type ) );
    self.m_TransportParameters.push_back( parameters );
    return args[0];
}


void MethodPy::PrintAll( ) const
{
    std::cout << "Method parameters\n";
    for( const auto& param : m_Parameters )
        std::cout << "Parameter: " << param.first << "\t Value: " << param.second << "\n";

    std::cout << "\n";
    std::cout << "Transport Parameters\n";

    for( const auto& transportParameters : m_TransportParameters )
    {
        std::cout << "Transport:\n";
    	for( const auto& param : transportParameters )
            std::cout << "Parameter: " << param.first << "\t Value: " << param.second << "\n";

        std::cout << "\n";
    }
}




} //end namespace

