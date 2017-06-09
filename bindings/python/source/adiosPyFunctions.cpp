/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosPyFunctions.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosPyFunctions.h"

namespace adios
{

Dims PyListToDims(const pyList list) noexcept
{
    const unsigned int length = pybind11::len(list);
    Dims dimensions;
    dimensions.reserve(length);

    for (unsigned int i = 0; i < length; ++i)
    {
        dimensions.push_back(pybind11::cast<size_t>(list[i]));
    }

    return dimensions;
}

Params KwargsToParams(const pyKwargs &kwargs) noexcept
{
    Params parameters;

    for (const auto &pair : kwargs)
    {
        parameters.emplace(pybind11::cast<std::string>(pair.first),
                           pybind11::cast<std::string>(pair.second));
    }
    return parameters;
}

} // end namespace adios
