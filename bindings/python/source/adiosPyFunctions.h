/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosPyFunctions.h
 *
 *  Created on: Mar 13, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_SOURCE_ADIOSPYFUNCTIONS_H_
#define ADIOS2_BINDINGS_PYTHON_SOURCE_ADIOSPYFUNCTIONS_H_

#include <map>
#include <string>
#include <vector>

#include "adios2/ADIOSTypes.h"
#include "adiosPyTypes.h"

namespace adios
{

/**
 * Python list to vector of dimensions (Dims)
 * @param list python list of numbers
 * @return adios::Dims
 */
Dims PyListToDims(const pyList list);

/**
 * Python dictionary kwargs to adios::Params (std::map<std::string,
 * std::string>)
 * @param kwargs dictionary
 * @return adios::Params
 */
Params KwargsToParams(const pyKwargs &kwargs);

} // end namespace adios

#endif /* BINDINGS_PYTHON_SOURCE_ADIOSPYFUNCTIONS_H_ */
