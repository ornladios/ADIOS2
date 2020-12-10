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
#include "adios2/helper/adiosFunctions.h" //GetDataType<T>

#include <type_traits>

namespace adios2
{
namespace core
{

namespace // anonymous
{

template <class T>
struct RequiresZeroPadding : std::false_type
{
};

template <>
struct RequiresZeroPadding<long double> : std::true_type
{
};
}

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(const Attribute<T> &other)                         \
    : AttributeBase(other), m_DataArray(other.m_DataArray)                     \
    {                                                                          \
        if (RequiresZeroPadding<T>::value)                                     \
            std::memset(&m_DataSingleValue, 0, sizeof(m_DataSingleValue));     \
        m_DataSingleValue = other.m_DataSingleValue;                           \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(const std::string &name, const T *array,           \
                            const size_t elements)                             \
    : AttributeBase(name, helper::GetDataType<T>(), elements)                  \
    {                                                                          \
        if (RequiresZeroPadding<T>::value)                                     \
            std::memset(&m_DataSingleValue, 0, sizeof(m_DataSingleValue));     \
        m_DataArray = std::vector<T>(array, array + elements);                 \
    }                                                                          \
                                                                               \
    template <>                                                                \
    Attribute<T>::Attribute(const std::string &name, const T &value)           \
    : AttributeBase(name, helper::GetDataType<T>())                            \
    {                                                                          \
        if (RequiresZeroPadding<T>::value)                                     \
            std::memset(&m_DataSingleValue, 0, sizeof(m_DataSingleValue));     \
        m_DataSingleValue = value;                                             \
    }

ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type

} // end namespace core
} // end namespace adios2
