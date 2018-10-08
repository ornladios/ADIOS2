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

#ifdef __cplusplus
extern "C" {
#endif

namespace
{
const std::map<std::string, std::vector<adios2_type>>
    adios2_attribute_types_map = {
        {"char", {adios2_type_char}},
        {"int", {adios2_type_int32_t, adios2_type_int}},
        {"float", {adios2_type_float}},
        {"double", {adios2_type_double}},
        {"signed char", {adios2_type_int8_t, adios2_type_signed_char}},
        {"short", {adios2_type_int16_t, adios2_type_short}},
        {"long int", {adios2_type_int64_t, adios2_type_long_int}},
        {"long long int", {adios2_type_int64_t, adios2_type_long_long_int}},
        {"string", {adios2_type_string}},
        {"string array", {adios2_type_string_array}},
        {"unsigned char", {adios2_type_unsigned_char, adios2_type_uint8_t}},
        {"unsigned short", {adios2_type_unsigned_short, adios2_type_uint16_t}},
        {"unsigned int", {adios2_type_unsigned_int, adios2_type_uint32_t}},
        {"unsigned long int",
         {adios2_type_unsigned_long_int, adios2_type_uint64_t}},
        {"unsigned long long int",
         {adios2_type_unsigned_long_long_int, adios2_type_uint64_t}},
};
} // end anonymous namespace

adios2_error adios2_attribute_name(char *name, size_t *size,
                                   const adios2_attribute *attribute)
{
    try
    {
        adios2::helper::CheckForNullptr(
            attribute, "for attribute, in call to adios2_attribute_name");
        adios2::helper::CheckForNullptr(
            name, "for char* char, in call to adios2_attribute_name");

        const adios2::core::AttributeBase *attributeBase =
            reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

        *size = attributeBase->m_Name.size();
        attributeBase->m_Name.copy(name, *size);
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_attribute_name"));
    }
}

adios2_error adios2_attribute_type(adios2_type *type,
                                   const adios2_attribute *attribute)
{
    try
    {
        adios2::helper::CheckForNullptr(attribute,
                                        "in call to adios2_attribute_type");
        const adios2::core::AttributeBase *attributeBase =
            reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

        auto itType = adios2_attribute_types_map.find(attributeBase->m_Type);
        *type = (itType == adios2_attribute_types_map.end())
                    ? adios2_type_unknown
                    : itType->second.front();
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_attribute_type"));
    }
}

adios2_error adios2_attribute_data(void *data, size_t *size,
                                   const adios2_attribute *attribute)
{
    try
    {
        adios2::helper::CheckForNullptr(attribute,
                                        "in call to adios2_attribute_data");
        adios2::helper::CheckForNullptr(
            data, "for data, in call to adios2_attribute_data");

        const adios2::core::AttributeBase *attributeBase =
            reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

        const std::string type(attributeBase->m_Type);

        if (type == "")
        {
            // not supported
        }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::helper::GetType<T>())                             \
    {                                                                          \
        const adios2::core::Attribute<T> *attributeCpp =                       \
            dynamic_cast<const adios2::core::Attribute<T> *>(attributeBase);   \
        T *dataT = reinterpret_cast<T *>(data);                                \
        if (attributeCpp->m_IsSingleValue)                                     \
        {                                                                      \
            *dataT = attributeCpp->m_DataSingleValue;                          \
            *size = 1;                                                         \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            std::copy(attributeCpp->m_DataArray.begin(),                       \
                      attributeCpp->m_DataArray.end(), dataT);                 \
            *size = attributeCpp->m_Elements;                                  \
        }                                                                      \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_attribute_data"));
    }
}

#ifdef __cplusplus
} // end extern C
#endif
