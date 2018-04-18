/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx03Attribute.tcc
 *
 *  Created on: Apr 10, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX03_CXX03_CXX03ATTRIBUTE_TCC_
#define BINDINGS_CXX03_CXX03_CXX03ATTRIBUTE_TCC_

#include "cxx03Attribute.h"
#include "cxx03types.h"

namespace adios2
{
namespace cxx03
{

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(adios2_attribute *attribute)                       \
    : m_Attribute(attribute)                                                   \
    {                                                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Attribute<T>::~Attribute()                                                 \
    {                                                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Attribute<T>::operator bool() const                                        \
    {                                                                          \
        if (m_Attribute == NULL)                                               \
        {                                                                      \
            return false;                                                      \
        }                                                                      \
        return true;                                                           \
    }

ADIOS2_FOREACH_CXX03_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
}
}

#endif /* BINDINGS_CXX03_CXX03_CXX03ATTRIBUTE_TCC_ */
