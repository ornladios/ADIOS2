/*
 * adiosPyFunctions.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */
#include <iostream>

#include "adiosPyFunctions.h"


namespace adios
{

#ifdef HAVE_BOOSTPYTHON
namespace py = boost::python;
using pyCastString = py::extract<std::string>;
using pyCastSize_t = py::extract<std::size_t>;

#endif

#ifdef HAVE_PYBIND11
namespace py = pybind11;
using pyCastString = pybind11::cast<std::string>;
using pyCastSize_t = pybind11::cast<std::size_t>;
#endif



Dims ListToVector( const pyList& list )
{
    const unsigned int length = py::len( list );
    Dims vec;
    vec.reserve( length );

    for( unsigned int i=0; i<length;i++ )
        vec.push_back( pyCastSize_t( list[i]) );

    return vec;
}


std::map<std::string, std::string> DictToMap( const pyDict& dictionary )
{
    pyList keys = dictionary.keys();
    const unsigned int length = py::len( keys );

    std::map<std::string, std::string> parameters;

    for( unsigned int k = 0; k < length; ++k )
    {
        const std::string key( pyCastString( keys[k] ) );
        const std::string value( pyCastString( dictionary[ keys[k] ] ) );
        parameters.insert( std::make_pair( key, value ) );
    }

    return parameters;
}


} //end namespace

