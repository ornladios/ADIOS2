/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.inl
 *
 *  Created on: May 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_VARIABLE_INL_
#define ADIOS2_CORE_VARIABLE_INL_
#ifndef ADIOS2_CORE_VARIABLE_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include "Variable.h"

#include "adios2/helper/adiosFunctions.h" //GetType

namespace adios
{

template <class T>
Variable<T>::Variable(const std::string &name, const Dims shape,
                      const Dims start, const Dims count,
                      const bool constantShape, const bool debugMode)
: VariableBase(name, GetType<T>(), sizeof(T), shape, start, count,
               constantShape, debugMode)
{
}

template <class T>
void Variable<T>::ApplyTransforms()
{
}

} // end namespace adios

#endif /* ADIOS2_CORE_VARIABLE_INL_ */
