/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_attribute.cpp :
 *
 *  Created on: Jun 11, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "adios2_c_attribute.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/core/AttributeBase.h"
#include "adios2/helper/adiosFunctions.h"

const char *adios2_attribute_name(const adios2_attribute *attribute,
                                  size_t *size)
{
    adios2::helper::CheckForNullptr(attribute,
                                    "in call to adios2_attribute_name");
    const adios2::core::AttributeBase *attributeBase =
        reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

    return attributeBase->m_Name.c_str();
}

const char *adios2_attribute_type(const adios2_attribute *attribute,
                                  size_t *size)
{
    adios2::helper::CheckForNullptr(attribute,
                                    "in call to adios2_attribute_type");
    const adios2::core::AttributeBase *attributeBase =
        reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

    *size = attributeBase->m_Type.size();
    return attributeBase->m_Type.c_str();
}

const void *adios2_attribute_data(const adios2_attribute *attribute,
                                  size_t *size)
{
    adios2::helper::CheckForNullptr(attribute,
                                    "in call to adios2_attribute_data");
    adios2::helper::CheckForNullptr(
        attribute, "for *size, in call to adios2_attribute_data");

    const adios2::core::AttributeBase *attributeBase =
        reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

    const std::string type(attributeBase->m_Type);

    void *data = nullptr;
    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        const adios2::core::Attribute<T> *attributeCpp =                       \
            reinterpret_cast<const adios2::core::Attribute<T> *>(attribute);   \
        if (attributeCpp->m_IsSingleValue)                                     \
        {                                                                      \
            data = reinterpret_cast<void *>(                                   \
                const_cast<T *>(&attributeCpp->m_DataSingleValue));            \
            *size = 1;                                                         \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            data = reinterpret_cast<void *>(                                   \
                const_cast<T *>(attributeCpp->m_DataArray.data()));            \
            *size = attributeCpp->m_Elements;                                  \
        }                                                                      \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    return data;
}
