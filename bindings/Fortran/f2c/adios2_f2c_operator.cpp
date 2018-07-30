/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_operator.cpp :
 *
 *  Created on: Jul 30, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_f2c_operator.h"

#include <cstddef>  //std::size_t
#include <iostream> //std::cerr
#include <stdexcept>

void FC_GLOBAL(adios2_operator_type_f2c,
               ADIOS2_OPERATOR_TYPE_F2C)(const adios2_operator **op,
                                         char name[1024], int *length,
                                         int *ierr)
{
    *ierr = 0;
    try
    {
        std::size_t lengthC = 0;
        const char *nameC = adios2_operator_type(*op, &lengthC);

        if (nameC == nullptr)
        {
            throw std::runtime_error("ERROR: null pointer\n");
        }

        for (std::size_t i = 0; i < lengthC; ++i)
        {
            name[i] = nameC[i];
        }

        *length = static_cast<int>(lengthC);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 operator_type: " << e.what() << "\n";
        *ierr = -1;
    }
}
