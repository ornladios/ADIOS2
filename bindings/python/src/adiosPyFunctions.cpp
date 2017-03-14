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

Dims ListToVector( const boost::python::list& list )
{
    const boost::python::ssize_t length = boost::python::len( list );
    Dims vec;
    vec.reserve( length );

    for( unsigned int i=0; i<length;i++ )
        vec.push_back( boost::python::extract<std::size_t>( list[i]) );

    return vec;
}

std::map<std::string, std::string> DictToMap( const boost::python::dict& dictionary )
{
    boost::python::list keys = dictionary.keys();
    unsigned int length = boost::python::len( keys );

    std::map<std::string, std::string> parameters;

    for( unsigned int k = 0; k < length; ++k )
    {
        const std::string key( boost::python::extract<std::string>( keys[k] ) );
        const std::string value( boost::python::extract<std::string>( dictionary[ keys[k] ] ) );
        parameters.insert( std::make_pair( key, value ) );
    }

    return parameters;
}



} //end namespace

