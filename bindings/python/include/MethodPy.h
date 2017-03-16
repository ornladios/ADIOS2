/*
 * MethodPy.h
 *
 *  Created on: Mar 14, 2017
 *      Author: wfg
 */

#ifndef METHODPY_H_
#define METHODPY_H_

#ifdef HAVE_BOOSTPYTHON
  #include "boost/python.hpp"
#endif

#ifdef HAVE_PYBIND11
  #include "pybind11/pybind11.h"
  #include "pybind11/cast.h"
#endif


#include "core/Method.h"

namespace adios
{

#ifdef HAVE_BOOSTPYTHON
using pyObject = boost::python::object;
using pyTuple = boost::python::tuple;
using pyDict = boost::python::dict;
#endif

#ifdef HAVE_PYBIND11
using pyObject = pybind11::object;
using pyTuple = pybind11::tuple;
using pyDict = pybind11::dict;
#endif


class MethodPy : public Method
{

public:

    MethodPy( const std::string type, const bool debugMode );

    ~MethodPy( );

    /**
     * static needed to support raw function
     * @param dictionary
     * @return
     */
    static pyObject SetParametersPy( pyTuple args, pyDict kwargs );

    static pyObject AddTransportPy( pyTuple args, pyDict kwargs );

    void PrintAll( ) const;

};


}


#endif /* METHODPY_H_ */
