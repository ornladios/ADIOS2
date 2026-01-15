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

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/AttributeBase.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2_c_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

adios2_error adios2_attribute_name(char *name, size_t *size, const adios2_attribute *attribute)
{
    try
    {
        adios2::helper::CheckForNullptr(attribute,
                                        "for attribute, in call to adios2_attribute_name");

        const adios2::core::AttributeBase *attributeBase =
            reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

        return String2CAPI(attributeBase->m_Name, name, size);
    }
    catch (...)
    {
        return static_cast<adios2_error>(adios2::helper::ExceptionToError("adios2_attribute_name"));
    }
}

adios2_error adios2_attribute_type(adios2_type *type, const adios2_attribute *attribute)
{
    try
    {
        adios2::helper::CheckForNullptr(attribute, "in call to adios2_attribute_type");
        const adios2::core::AttributeBase *attributeBase =
            reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

        auto type_s = attributeBase->m_Type;
        if (type_s == adios2::helper::GetDataType<std::string>())
        {
            *type = adios2_type_string;
        }
#define make_case(T)                                                                               \
    else if (type_s == adios2::helper::GetDataType<MapAdios2Type<T>::Type>())                      \
    {                                                                                              \
        *type = T;                                                                                 \
    }
        ADIOS2_FOREACH_C_ATTRIBUTE_TYPE_1ARG(make_case)
#undef make_case
        else
        {
            *type = adios2_type_unknown;
        }
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(adios2::helper::ExceptionToError("adios2_attribute_type"));
    }
}

adios2_error adios2_attribute_type_string(char *type, size_t *size,
                                          const adios2_attribute *attribute)
{
    try
    {
        adios2::helper::CheckForNullptr(attribute, "for const adios2_attribute, in call to "
                                                   "adios2_attribute_type_string");
        adios2::helper::CheckForNullptr(
            size, "for size_t* length, in call to adios2_attribute_type_string");

        const adios2::core::AttributeBase *attributeBase =
            reinterpret_cast<const adios2::core::AttributeBase *>(attribute);
        return String2CAPI(ToString(attributeBase->m_Type), type, size);
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_attribute_type_string"));
    }
}

adios2_error adios2_attribute_is_value(adios2_bool *result, const adios2_attribute *attribute)
{
    try
    {
        adios2::helper::CheckForNullptr(attribute, "in call to adios2_attribute_is_value");
        const adios2::core::AttributeBase *attributeBase =
            reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

        *result = attributeBase->m_IsSingleValue ? adios2_true : adios2_false;
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_attribute_is_value"));
    }
}

adios2_error adios2_attribute_size(size_t *size, const adios2_attribute *attribute)
{
    try
    {
        adios2::helper::CheckForNullptr(attribute, "in call to adios2_attribute_size");
        const adios2::core::AttributeBase *attributeBase =
            reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

        *size = attributeBase->m_IsSingleValue ? 1 : attributeBase->m_Elements;
        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(adios2::helper::ExceptionToError("adios2_attribute_size"));
    }
}

adios2_error adios2_attribute_data(void *data, size_t *size, const adios2_attribute *attribute)
{
    try
    {
        adios2::helper::CheckForNullptr(attribute, "in call to adios2_attribute_data");
        adios2::helper::CheckForNullptr(data, "for data, in call to adios2_attribute_data");

        const adios2::core::AttributeBase *attributeBase =
            reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

        const adios2::DataType type(attributeBase->m_Type);

        if (type == adios2::DataType::None)
        {
            // not supported
        }
        else if (type == adios2::helper::GetDataType<std::string>())
        {
            const adios2::core::Attribute<std::string> *attributeCpp =
                dynamic_cast<const adios2::core::Attribute<std::string> *>(attributeBase);
            if (attributeCpp->m_IsSingleValue)
            {
                char *dataT = reinterpret_cast<char *>(data);
                attributeCpp->m_DataSingleValue.copy(dataT, attributeCpp->m_DataSingleValue.size());
                dataT[attributeCpp->m_DataSingleValue.size()] = '\0';
                *size = 1;
            }
            else
            {
                *size = attributeCpp->m_Elements;
                char **dataT = reinterpret_cast<char **>(data);

                for (size_t e = 0; e < *size; ++e)
                {
                    attributeCpp->m_DataArray[e].copy(dataT[e],
                                                      attributeCpp->m_DataArray[e].size());
                    dataT[e][attributeCpp->m_DataArray[e].size()] = '\0';
                }
            }
        }
#define declare_template_instantiation(T)                                                          \
    else if (type == adios2::helper::GetDataType<T>())                                             \
    {                                                                                              \
        const adios2::core::Attribute<T> *attributeCpp =                                           \
            dynamic_cast<const adios2::core::Attribute<T> *>(attributeBase);                       \
        T *dataT = reinterpret_cast<T *>(data);                                                    \
        if (attributeCpp->m_IsSingleValue)                                                         \
        {                                                                                          \
            *dataT = attributeCpp->m_DataSingleValue;                                              \
            *size = 1;                                                                             \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            std::copy(attributeCpp->m_DataArray.begin(), attributeCpp->m_DataArray.end(), dataT);  \
            *size = attributeCpp->m_Elements;                                                      \
        }                                                                                          \
    }
        ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(adios2::helper::ExceptionToError("adios2_attribute_data"));
    }
}

adios2_error adios2_attribute_string_data(const adios2_attribute *attribute, char *data,
                                          size_t *length)
{
    try
    {
        adios2::helper::CheckForNullptr(attribute,
                                        "for attribute, in call to adios2_attribute_string_data");
        adios2::helper::CheckForNullptr(
            length, "for size_t *length, in call to adios2_attribute_string_data");

        const adios2::core::AttributeBase *attributeBase =
            reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

        if (attributeBase->m_Type != adios2::helper::GetDataType<std::string>())
        {
            throw std::invalid_argument("ERROR: attribute " + attributeBase->m_Name +
                                        " is not of string type, in call to "
                                        "adios2_attribute_string_data\n");
        }

        if (!attributeBase->m_IsSingleValue)
        {
            throw std::invalid_argument("ERROR: attribute " + attributeBase->m_Name +
                                        " is not a single value, use "
                                        "adios2_attribute_string_data_array instead, in call to "
                                        "adios2_attribute_string_data\n");
        }

        const adios2::core::Attribute<std::string> *attributeCpp =
            dynamic_cast<const adios2::core::Attribute<std::string> *>(attributeBase);

        *length = attributeCpp->m_DataSingleValue.size();

        if (data != nullptr)
        {
            attributeCpp->m_DataSingleValue.copy(data, *length);
        }

        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_attribute_string_data"));
    }
}

adios2_error adios2_attribute_string_data_array(const adios2_attribute *attribute, char **data,
                                                size_t *lengths)
{
    try
    {
        adios2::helper::CheckForNullptr(
            attribute, "for attribute, in call to adios2_attribute_string_data_array");
        adios2::helper::CheckForNullptr(
            lengths, "for size_t *lengths, in call to adios2_attribute_string_data_array");

        const adios2::core::AttributeBase *attributeBase =
            reinterpret_cast<const adios2::core::AttributeBase *>(attribute);

        if (attributeBase->m_Type != adios2::helper::GetDataType<std::string>())
        {
            throw std::invalid_argument("ERROR: attribute " + attributeBase->m_Name +
                                        " is not of string type, in call to "
                                        "adios2_attribute_string_data_array\n");
        }

        if (attributeBase->m_IsSingleValue)
        {
            throw std::invalid_argument("ERROR: attribute " + attributeBase->m_Name +
                                        " is a single value, use "
                                        "adios2_attribute_string_data instead, in call to "
                                        "adios2_attribute_string_data_array\n");
        }

        const adios2::core::Attribute<std::string> *attributeCpp =
            dynamic_cast<const adios2::core::Attribute<std::string> *>(attributeBase);

        const size_t count = attributeCpp->m_Elements;

        for (size_t i = 0; i < count; ++i)
        {
            lengths[i] = attributeCpp->m_DataArray[i].size();

            if (data != nullptr)
            {
                attributeCpp->m_DataArray[i].copy(data[i], lengths[i]);
            }
        }

        return adios2_error_none;
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_attribute_string_data_array"));
    }
}

#ifdef __cplusplus
} // end extern C
#endif
