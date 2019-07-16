/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_operator.cpp :
 *
 *  Created on: Jul 30, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_c_operator.h"

#include "adios2/helper/adiosFunctions.h"
#include "adios2_c_internal.h"

adios2_error adios2_operator_type(char *type, size_t *size,
                                  const adios2_operator *op)
{
    try
    {
        adios2::helper::CheckForNullptr(
            op, "for adios2_operator, in call to adios2_operator_type");

        const adios2::core::Operator *opCpp =
            reinterpret_cast<const adios2::core::Operator *>(op);

        return String2CAPI(opCpp->m_Type, type, size);
    }
    catch (...)
    {
        return static_cast<adios2_error>(
            adios2::helper::ExceptionToError("adios2_operator_type"));
    }
}
