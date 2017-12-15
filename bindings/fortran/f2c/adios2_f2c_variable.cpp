/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_variable.cpp
 *
 *  Created on: Nov 12, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_f2c_variable.h"

#include <cstddef>
#include <stdexcept>
#include <string.h> //strcpy
#include <vector>

void FC_GLOBAL(adios2_variable_name_f2c,
               ADIOS2_VARIABLE_NAME_F2C)(const adios2_Variable **variable,
                                         char name[1024], int *length,
                                         int *ierr)
{
    *ierr = 0;
    try
    {
        size_t lengthC = 0;
        const char *nameC = adios2_variable_name(*variable, &lengthC);

        if (nameC == nullptr)
        {
            throw std::runtime_error("ERROR: null pointer\n");
        }

        for (size_t i = 0; i < lengthC; ++i)
        {
            name[i] = nameC[i];
        }

        *length = static_cast<int>(lengthC);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_variable_type_f2c,
               ADIOS2_VARIABLE_TYPE_F2C)(const adios2_Variable **variable,
                                         int *type, int *ierr)
{
    *ierr = 0;
    try
    {
        *type = static_cast<int>(adios2_variable_type(*variable));
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_set_shape_f2c,
               ADIOS2_SET_SHAPE_F2C)(adios2_Variable **variable,
                                     const int *ndims, const int *shape,
                                     int *ierr)
{
    *ierr = 0;
    if (shape == nullptr || ndims == nullptr)
    {
        *ierr = 1;
        return;
    }

    try
    {
        std::vector<std::size_t> shapeV(shape, shape + *ndims);
        adios2_set_shape(*variable, shapeV.size(), shapeV.data());
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_set_selection_f2c,
               ADIOS2_SET_SELECTION_F2C)(adios2_Variable **variable,
                                         const int *ndims, const int *start,
                                         const int *count, int *ierr)
{
    auto lf_IntToSizeT = [](const int *dimensions, const int size,
                            std::vector<std::size_t> &output,
                            const bool offset) {

        if (dimensions == nullptr)
        {
            return;
        }

        output.resize(size);

        if (offset)
        {
            for (unsigned int d = 0; d < size; ++d)
            {
                output[d] = dimensions[d] - 1;
            }
        }
        else
        {
            for (unsigned int d = 0; d < size; ++d)
            {
                output[d] = dimensions[d];
            }
        }
    };

    *ierr = 0;
    if (start == nullptr || count == nullptr)
    {
        *ierr = 1;
        return;
    }

    try
    {
        std::vector<std::size_t> startV, countV;
        lf_IntToSizeT(start, *ndims, startV, true);
        lf_IntToSizeT(count, *ndims, countV, false);
        adios2_set_selection(*variable, *ndims, startV.data(), countV.data());
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}
