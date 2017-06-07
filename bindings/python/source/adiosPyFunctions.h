/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosPyFunctions.h
 *
 *  Created on: Mar 13, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_PYTHON_SOURCE_ADIOSPYFUNCTIONS_H_
#define BINDINGS_PYTHON_SOURCE_ADIOSPYFUNCTIONS_H_

#include <map>
#include <string>
#include <vector>

#include "adios2/ADIOSTypes.h"
#include "bindings/python/source/adiosPyTypes.h"

namespace adios
{

/**
 * Transforms a boost python list to a Dims (std::vector<std::size_t>) object
 * @param list input boost python list from python program
 * @return Dims (std::vector<std::size_t>) object than can be passed to python
 */
Dims ListToVector(const pyList &list);

std::map<std::string, std::string> KwargsToMap(const pyKwargs &dictionary);

template <class T>
const T *PyArrayToPointer(const pyArray &array)
{

    return reinterpret_cast<const T *>(array.data());
}

template <class T>
bool IsType(const pyArray &array)
{
    bool isType = false;

    if (pybind11::isinstance<pybind11::array_t<T>>(array))
    {
        isType = true;
    }

    return isType;
}

template <class T, class U>
T PyCast(U object)
{
    return pybind11::cast<T>(object);
}

bool IsEmpty(pyObject object);

} // end namespace adios

#endif /* BINDINGS_PYTHON_SOURCE_ADIOSPYFUNCTIONS_H_ */
