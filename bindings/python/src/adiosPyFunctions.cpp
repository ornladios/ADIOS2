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

std::vector<std::size_t> ListToVector( const boost::python::list& list )
{
    const boost::python::ssize_t length = boost::python::len( list );
    std::vector<std::size_t> vec;
    vec.reserve( length );

    for( unsigned int i=0; i<length;i++ )
        vec.push_back( boost::python::extract<std::size_t>( list[i]) );

    return vec;
}


boost::python::list VectorToList( const std::vector<std::size_t>& vec )
{
    boost::python::list list;

    for( auto vecElement : vec )
    {
        list.append( vecElement );
    }

    return list;
}





}

