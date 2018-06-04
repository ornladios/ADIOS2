/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98Attribute.cpp
 *
 *  Created on: Apr 10, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "cxx98Attribute.h"
#include "cxx98types.h"

#include <adios2_c.h>

namespace adios2
{
namespace cxx98
{

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute() : m_Attribute(NULL)                              \
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
        return m_Attribute == NULL ? false : true;                             \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::string Attribute<T>::Name() const                                     \
    {                                                                          \
        size_t size = 0;                                                       \
        const char *name = adios2_attribute_name(m_Attribute, &size);          \
        return std::string(name, size);                                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::string Attribute<T>::Type() const                                     \
    {                                                                          \
        size_t size = 0;                                                       \
        const char *type = adios2_attribute_type(m_Attribute, &size);          \
        return std::string(type, size);                                        \
    }                                                                          \
                                                                               \
    template <>                                                                \
    std::vector<T> Attribute<T>::Data() const                                  \
    {                                                                          \
        size_t size = 0;                                                       \
        const T *type = (const T *)adios2_attribute_data(m_Attribute, &size);  \
        return std::vector<T>(type, type + size);                              \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(adios2_attribute *attribute)                       \
    : m_Attribute(attribute)                                                   \
    {                                                                          \
    }

ADIOS2_FOREACH_CXX98_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace cxx98
} // end namespace adios2
