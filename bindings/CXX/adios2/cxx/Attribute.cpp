/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Attribute.h"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/Attribute.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

#define declare_type(T)                                                                            \
                                                                                                   \
    template <>                                                                                    \
    Attribute<T>::Attribute(core::Attribute<IOType> *attribute) : m_Attribute(attribute)           \
    {                                                                                              \
    }                                                                                              \
                                                                                                   \
    template <>                                                                                    \
    Attribute<T>::operator bool() const noexcept                                                   \
    {                                                                                              \
        return (m_Attribute == nullptr) ? false : true;                                            \
    }                                                                                              \
                                                                                                   \
    template <>                                                                                    \
    std::string Attribute<T>::Name() const                                                         \
    {                                                                                              \
        helper::CheckForNullptr(m_Attribute, "in call to Attribute<T>::Name()");                   \
        return m_Attribute->m_Name;                                                                \
    }                                                                                              \
                                                                                                   \
    template <>                                                                                    \
    std::string Attribute<T>::Type() const                                                         \
    {                                                                                              \
        helper::CheckForNullptr(m_Attribute, "in call to Attribute<T>::Type()");                   \
        return ToString(m_Attribute->m_Type);                                                      \
    }                                                                                              \
                                                                                                   \
    template <>                                                                                    \
    std::vector<T> Attribute<T>::Data() const                                                      \
    {                                                                                              \
        helper::CheckForNullptr(m_Attribute, "in call to Attribute<T>::Data()");                   \
                                                                                                   \
        if (m_Attribute->m_IsSingleValue)                                                          \
        {                                                                                          \
            return std::vector<T>{m_Attribute->m_DataSingleValue};                                 \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            return helper::NewVectorType<IOType, T>(m_Attribute->m_DataArray);                     \
        }                                                                                          \
    }                                                                                              \
                                                                                                   \
    template <>                                                                                    \
    bool Attribute<T>::IsValue() const                                                             \
    {                                                                                              \
        helper::CheckForNullptr(m_Attribute, "in call to Attribute<T>::IsValue()");                \
        return m_Attribute->m_IsSingleValue;                                                       \
    }                                                                                              \
                                                                                                   \
    template <>                                                                                    \
    std::string ToString(const Attribute<T> &attribute)                                            \
    {                                                                                              \
        return std::string("Attribute<") + attribute.Type() + ">(Name: \"" + attribute.Name() +    \
               "\")";                                                                              \
    }

ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2
