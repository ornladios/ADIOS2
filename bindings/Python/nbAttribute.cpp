/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11Variable.cpp :
 *
 *  Created on: Dec 11, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "nbAttribute.h"
#include "nbTypes.h"

#include <string.h> //std::memcpy

#include <nanobind/ndarray.h>

#include "adios2/helper/adiosFunctions.h"

namespace nb = nanobind;

namespace adios2
{
namespace py11
{

Attribute::Attribute(core::AttributeBase *attribute) : m_Attribute(attribute) {}

Attribute::operator bool() const noexcept { return (m_Attribute == nullptr) ? false : true; }

std::string Attribute::Name() const
{
    helper::CheckForNullptr(m_Attribute, "in call to Attribute::Name");
    return m_Attribute->m_Name;
}

std::string Attribute::Type() const
{
    helper::CheckForNullptr(m_Attribute, "in call to Attribute::Type");
    return ToString(m_Attribute->m_Type);
}

bool Attribute::SingleValue() const
{
    helper::CheckForNullptr(m_Attribute, "in call to Attribute::SingleValue");
    return m_Attribute->m_IsSingleValue;
}

std::vector<std::string> Attribute::DataString()
{
    helper::CheckForNullptr(m_Attribute, "in call to Attribute::DataStrings");
    const adios2::DataType type = m_Attribute->m_Type;

    std::vector<std::string> data;

    if (type == helper::GetDataType<std::string>())
    {
        const core::Attribute<std::string> *attribute =
            dynamic_cast<core::Attribute<std::string> *>(m_Attribute);

        data.reserve(attribute->m_Elements);
        if (attribute->m_IsSingleValue)
        {
            data.push_back(attribute->m_DataSingleValue);
        }
        else
        {
            data = attribute->m_DataArray;
        }
    }
    else
    {
        throw std::invalid_argument("ERROR: data type for attribute " + m_Attribute->m_Name +
                                    " is not string, in call to Attribute::DataStrings\n");
    }
    return data;
}

nb::object Attribute::Data()
{
    helper::CheckForNullptr(m_Attribute, "in call to Attribute::Data");
    const adios2::DataType type = m_Attribute->m_Type;

    if (type == adios2::DataType::Struct)
    {
        // not supported
    }
#define declare_type(T)                                                                            \
    else if (type == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        T *data = new T[m_Attribute->m_Elements];                                                  \
        if (m_Attribute->m_IsSingleValue)                                                          \
        {                                                                                          \
            const T value = dynamic_cast<core::Attribute<T> *>(m_Attribute)->m_DataSingleValue;    \
            data[0] = value;                                                                       \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            const std::vector<T> &values =                                                         \
                dynamic_cast<core::Attribute<T> *>(m_Attribute)->m_DataArray;                      \
            std::memcpy(data, values.data(), sizeof(T) * m_Attribute->m_Elements);                 \
        }                                                                                          \
        nb::capsule owner(data, [](void *p) noexcept { delete[] static_cast<T *>(p); });           \
        size_t shape = m_Attribute->m_Elements;                                                    \
        auto arr = nb::ndarray<nb::numpy, T, nb::c_contig>(data, 1, &shape, owner);                \
        return nb::cast(std::move(arr));                                                           \
    }
    ADIOS2_FOREACH_NUMPY_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
    return nb::none();
}

} // end namespace py11
} // end namespace adios2
