/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
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
#include "pybind11/cast.h"
#include "pybind11/pybind11.h"
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
  MethodPy(const std::string type, const bool debugMode);

  ~MethodPy();

#ifdef HAVE_BOOSTPYTHON
  static pyObject SetParametersPy(pyTuple args, pyDict kwargs);
  static pyObject AddTransportPy(pyTuple args, pyDict kwargs);
#endif

#ifdef HAVE_PYBIND11
  void SetParametersPyBind11(pybind11::kwargs kwargs);
  void AddTransportPyBind11(const std::string type, pybind11::kwargs kwargs);
#endif

  void PrintAll() const;
};
}

#endif /* METHODPY_H_ */
