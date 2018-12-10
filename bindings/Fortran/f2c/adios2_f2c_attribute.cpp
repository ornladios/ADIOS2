/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_attribute.cpp
 *
 *  Created on: Dec 10, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_f2c_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void FC_GLOBAL(adios2_attribute_is_value_f2c, ADIOS2_ATTRIBUTE_IS_VALUE_F2C)(
    int *is_value, const adios2_attribute **attribute, int *ierr)
{
    adios2_bool isValueC;
    *ierr = static_cast<int>(adios2_attribute_is_value(&isValueC, *attribute));
    if (*ierr == static_cast<int>(adios2_error_none))
    {
        *is_value = (isValueC == adios2_true) ? 1 : 0;
    }
}

void FC_GLOBAL(adios2_attribute_type_f2c,
               ADIOS2_ATTRIBUTE_TYPE_F2C)(int *type,
                                          const adios2_attribute **attribute,
                                          int *ierr)
{
    *type = -1;
    adios2_type typeC;
    *ierr = static_cast<int>(adios2_attribute_type(&typeC, *attribute));
    if (*ierr == static_cast<int>(adios2_error_none))
    {
        *type = static_cast<int>(typeC);
    }
}

void FC_GLOBAL(adios2_attribute_length_f2c,
               ADIOS2_ATTRIBUTE_LENGTH_F2C)(int *length,
                                            const adios2_attribute **attribute,
                                            int *ierr)
{
    *length = -1;
    size_t lengthC;
    *ierr = static_cast<int>(adios2_attribute_size(&lengthC, *attribute));
    if (*ierr == static_cast<int>(adios2_error_none))
    {
        *length = static_cast<int>(lengthC);
    }
}

void FC_GLOBAL(adios2_attribute_value_f2c,
               ADIOS2_ATTRIBUTE_VALUE_F2C)(void *data,
                                           const adios2_attribute **attribute,
                                           int *ierr)
{
    size_t size = 0;
    *ierr = static_cast<int>(adios2_attribute_data(data, &size, *attribute));
}

void FC_GLOBAL(adios2_attribute_data_f2c,
               ADIOS2_ATTRIBUTE_DATA_F2C)(void *data, int *size,
                                          const adios2_attribute **attribute,
                                          int *ierr)
{
    *size = -1;
    size_t sizeC;
    *ierr = static_cast<int>(adios2_attribute_data(data, &sizeC, *attribute));
    if (*ierr == static_cast<int>(adios2_error_none))
    {
        *size = static_cast<int>(sizeC);
    }
}

#ifdef __cplusplus
}
#endif
