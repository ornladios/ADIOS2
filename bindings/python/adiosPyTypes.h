/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosPyTypes.h
 *
 *  Created on: Jun 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_SOURCE_ADIOSPYTYPES_H_
#define ADIOS2_BINDINGS_PYTHON_SOURCE_ADIOSPYTYPES_H_

#include <adios2.h>
#include <pybind11/numpy.h>
#include <pybind11/pytypes.h>

namespace adios2
{
// pytypes
using pyObject = pybind11::object;
using pyTuple = pybind11::tuple;
using pyDict = pybind11::dict;
using pyKwargs = pybind11::kwargs;
using pyList = pybind11::list;

// numpy
using pyArray = pybind11::array;
using pyDType = pybind11::dtype;
}

#endif /* BINDINGS_PYTHON_SOURCE_ADIOSPYTYPES_H_ */
