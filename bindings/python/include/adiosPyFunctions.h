/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosPyFunctions.h
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#ifndef ADIOSPYFUNCTIONS_H_
#define ADIOSPYFUNCTIONS_H_

#include <map>
#include <string>
#include <vector>

#ifdef HAVE_BOOSTPYTHON
#include "boost/python.hpp"
#include "boost/python/numpy.hpp"
#endif

#ifdef HAVE_PYBIND11
#include "pybind11/numpy.h"
#include "pybind11/pybind11.h"
#endif

namespace adios
{

using Dims = std::vector<std::size_t>;

#ifdef HAVE_BOOSTPYTHON
using pyList = boost::python::list;
using pyDict = boost::python::dict;
using pyArray = boost::python::numpy::ndarray;
using dtype = boost::python::numpy::dtype;
using pyKwargs = boost::python::dict;
using pyObject = boost::python::object;
#endif

#ifdef HAVE_PYBIND11
using pyList = pybind11::list;
using pyDict = pybind11::dict;
using pyArray = pybind11::array;
using dtype = pybind11::dtype;
using pyKwargs = pybind11::kwargs;
using pyObject = pybind11::object;
#endif

/**
 * Transforms a boost python list to a Dims (std::vector<std::size_t>) object
 * @param list input boost python list from python program
 * @return Dims (std::vector<std::size_t>) object than can be passed to python
 */
Dims ListToVector(const pyList &list);

#ifdef HAVE_BOOSTPYTHON
std::map<std::string, std::string> DictToMap(const pyDict &dictionary);
#endif

#ifdef HAVE_PYBIND11
std::map<std::string, std::string>
KwargsToMap(const pybind11::kwargs &dictionary);
#endif

template <class T>
const T *PyArrayToPointer(const pyArray &array)
{
#ifdef HAVE_BOOSTPYTHON
    return reinterpret_cast<const T *>(array.get_data());
#endif

#ifdef HAVE_PYBIND11
    return reinterpret_cast<const T *>(array.data());
#endif
}

template <class T>
bool IsType(const pyArray &array)
{
#ifdef HAVE_BOOSTPYTHON
    if (array.get_dtype() == dtype::get_builtin<T>())
        return true;
#endif

#ifdef HAVE_PYBIND11
    if (pybind11::isinstance<pybind11::array_t<T>>(array))
        return true;
#endif

    return false;
}

template <class T, class U>
T PyCast(U object)
{
#ifdef HAVE_BOOSTPYTHON
    return boost::python::extract<T>(object);
#endif

#ifdef HAVE_PYBIND11
    return pybind11::cast<T>(object);
#endif
}

bool IsEmpty(pyObject object);

} // end namespace

#endif /* ADIOSPYFUNCTIONS_H_ */
