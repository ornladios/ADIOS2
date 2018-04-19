/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98Attribute.tcc
 *
 *  Created on: Apr 10, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX98_CXX98_CXX98ATTRIBUTE_TCC_
#define BINDINGS_CXX98_CXX98_CXX98ATTRIBUTE_TCC_

#include "cxx98Attribute.h"
#include "cxx98types.h"

namespace adios2
{
namespace cxx98
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

ADIOS2_FOREACH_CXX98_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
}
}

#endif /* BINDINGS_CXX98_CXX98_CXX98ATTRIBUTE_TCC_ */
