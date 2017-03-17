/*
 * adiosPyFunctions.h
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#ifndef ADIOSPYFUNCTIONS_H_
#define ADIOSPYFUNCTIONS_H_

#include <vector>
#include <map>
#include <string>

#ifdef HAVE_BOOSTPYTHON
  #include "boost/python.hpp"
  #include "boost/python/numpy.hpp"
#endif

#ifdef HAVE_PYBIND11
  #include "pybind11/pybind11.h"
  #include "pybind11/numpy.h"
#endif


namespace adios
{

using Dims = std::vector<std::size_t>;

#ifdef HAVE_BOOSTPYTHON
using pyList = boost::python::list;
using pyDict = boost::python::dict;
using pyArray = boost::python::numpy::ndarray;
using dtype = boost::python::numpy::dtype;
#endif

#ifdef HAVE_PYBIND11
using pyList = pybind11::list;
using pyDict = pybind11::dict;
using pyArray = pybind11::array;
using dtype = pybind11::dtype;
#endif

/**
 * Transforms a boost python list to a Dims (std::vector<std::size_t>) object
 * @param list input boost python list from python program
 * @return Dims (std::vector<std::size_t>) object than can be passed to python
 */
Dims ListToVector( const pyList& list );

std::map<std::string, std::string> DictToMap( const pyDict& dictionary );

template< class T >
const T* PyArrayToPointer( const pyArray& array )
{
    #ifdef HAVE_BOOSTPYTHON
    return reinterpret_cast<const T*>( array.get_data() );
    #endif

    #ifdef HAVE_PYBIND11
    return reinterpret_cast<const T*>( array.data() );
    #endif
}


template< class T >
dtype GetDType( )
{
    #ifdef HAVE_BOOSTPYTHON
    return dtype::get_builtin<T>();
    #endif

    #ifdef HAVE_PYBIND11
    return dtype::of<T>();
    #endif
}

dtype DType( const pyArray& array );









} //end namespace



#endif /* ADIOSPYFUNCTIONS_H_ */
