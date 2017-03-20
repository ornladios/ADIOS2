/*
 * MethodPy.cpp
 *
 *  Created on: Mar 14, 2017
 *      Author: wfg
 */


#include <iostream>

#include "MethodPy.h"
#include "adiosPyFunctions.h"

namespace adios
{

#ifdef HAVE_BOOSTPYTHON
namespace py = boost::python;
#endif

#ifdef HAVE_PYBIND11
namespace py = pybind11;
#endif


MethodPy::MethodPy( const std::string type, const bool debugMode ):
    Method( type, debugMode )
{ }


MethodPy::~MethodPy( )
{ }

#ifdef HAVE_BOOSTPYTHON
pyObject MethodPy::SetParametersPy( pyTuple args, pyDict kwargs )
{
    if( py::len( args ) > 1  )
        throw std::invalid_argument( "ERROR: syntax of Method SetParameters function is incorrect, only use dictionary\n" );

    MethodPy& self = PyCast<MethodPy&>( args[0] );
    self.m_Parameters = DictToMap( kwargs );
    return args[0];
}


pyObject MethodPy::AddTransportPy( pyTuple args, pyDict kwargs )
{
    if( py::len( args ) != 2  )
        throw std::invalid_argument( "ERROR: syntax of Method AddTransport function is incorrect, only use one string for transport followed by a dictionary for parameters\n" );

    MethodPy& self = PyCast<MethodPy&>( args[0] );
    const std::string type = PyCast<std::string>( args[1] );

    auto parameters = DictToMap( kwargs );
    parameters.insert( std::make_pair( "transport", type ) );
    self.m_TransportParameters.push_back( parameters );
    return args[0];
}
#endif


#ifdef HAVE_PYBIND11
void MethodPy::SetParametersPyBind11( pybind11::kwargs kwargs )
{
    this->m_Parameters = KwargsToMap( kwargs );
}


void MethodPy::AddTransportPyBind11( const std::string type, pybind11::kwargs kwargs )
{
    auto parameters = KwargsToMap( kwargs );
    parameters.insert( std::make_pair( "transport", type ) );
    this->m_TransportParameters.push_back( parameters );
}
#endif



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

