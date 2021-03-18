/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Attribute.tcc
 *
 *  Created on: Oct 9, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ATTRIBUTE_TCC_
#define ADIOS2_CORE_ATTRIBUTE_TCC_

#include "Attribute.h"

#include "adios2/helper/adiosFunctions.h" //GetDataType<T>
#include "adios2/helper/adiosType.h"

#include <cstring>

#include <type_traits>

namespace adios2
{
namespace core
{

namespace // anonymous
{
template <typename T, typename Enable = void>
struct Pad;

template <typename T, typename Enable>
struct Pad
{
    static void Zero(T &arg) {}
};

template <typename T>
struct Pad<T, typename std::enable_if<std::is_trivial<T>::value>::type>
{
    static void Zero(T &arg) { std::memset(&arg, 0, sizeof(arg)); }
};

template <typename T>
struct Pad<T, typename std::enable_if<std::is_same<
                  T, std::complex<typename T::value_type>>::value>::type>
{
    static void Zero(T &arg)
    {
        Pad<typename T::value_type>::Zero(
            reinterpret_cast<typename T::value_type(&)[2]>(arg)[0]);
        Pad<typename T::value_type>::Zero(
            reinterpret_cast<typename T::value_type(&)[2]>(arg)[1]);
    }
};

}

template <typename T>
Attribute<T>::Attribute(const Attribute<T> &other)
: AttributeBase(other), m_DataArray(other.m_DataArray)
{
    Pad<T>::Zero(m_DataSingleValue);
    m_DataSingleValue = other.m_DataSingleValue;
}

template <typename T>
Attribute<T>::Attribute(const std::string &name, const T *array,
                        const size_t elements)
: AttributeBase(name, helper::GetDataType<T>(), elements)
{
    Pad<T>::Zero(m_DataSingleValue);
    m_DataArray = std::vector<T>(array, array + elements);
}

template <typename T>
Attribute<T>::Attribute(const std::string &name, const T &value)
: AttributeBase(name, helper::GetDataType<T>())
{
    Pad<T>::Zero(m_DataSingleValue);
    m_DataSingleValue = value;
}

template <typename T>
std::string Attribute<T>::DoGetInfoValue() const noexcept
{
    std::string value;
    if (m_IsSingleValue)
    {
        value = helper::ValueToString(m_DataSingleValue);
    }
    else
    {
        value = "{ " + helper::VectorToCSV(m_DataArray) + " }";
    }
    return value;
}

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_ATTRIBUTE_TCC_ */
