/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.tcc
 *
 *  Created on: Aug 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ATTRIBUTE_TCC_
#define ADIOS2_CORE_ATTRIBUTE_TCC_

#include "Attribute.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

namespace adios2
{

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(const std::string &name, const T *array,           \
                            const size_t elements)                             \
    : AttributeBase(name, GetType<T>(), elements, false),                      \
      m_DataArray(std::vector<T>(array, array + elements)),                    \
      m_DataSingleValue()                                                      \
    {                                                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(const std::string &name, const T &value)           \
    : AttributeBase(name, GetType<T>(), 1, true), m_DataSingleValue(value)     \
    {                                                                          \
    }

ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2

#endif /* ADIOS2_CORE_ATTRIBUTE_TCC_ */
