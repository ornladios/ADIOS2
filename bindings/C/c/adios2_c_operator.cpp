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

const char *adios2_operator_type(const adios2_operator *op, size_t *length)
{
    adios2::helper::CheckForNullptr(
        op, "for adios2_operator, in call to adios2_operator_type");

    const adios2::core::Operator *opCpp =
        reinterpret_cast<const adios2::core::Operator *>(op);

    *length = opCpp->m_Type.size();
    return opCpp->m_Type.c_str();
}
