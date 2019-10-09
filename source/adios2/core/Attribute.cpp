/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.cpp : needed for template separation using Attribute.tcc
 *
 *  Created on: Aug 3, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Attribute.h"
#include "Attribute.tcc"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

namespace adios2
{
namespace core
{

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(const std::string &name, const T *array,           \
                            const size_t elements)                             \
    : AttributeBase(name, helper::GetType<T>(), elements), m_DataSingleValue() \
    {                                                                          \
        m_DataArray = std::vector<T>(array, array + elements);                 \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(const std::string &name, const T &value)           \
    : AttributeBase(name, helper::GetType<T>()), m_DataSingleValue(value)      \
    {                                                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Params Attribute<T>::GetInfo() const noexcept                              \
    {                                                                          \
        return DoGetInfo();                                                    \
    }

ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type

} // end namespace core
} // end namespace adios2
