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
#include <vector>

void FC_GLOBAL(adios2_set_selection_f2c,
               ADIOS2_SET_SELECTION_F2C)(adios2_Variable **variable,
                                         const int *ndims, const int *start,
                                         const int *count, int *ierr)
{
    *ierr = 0;
    if (start == nullptr || count == nullptr)
    {
        *ierr = 1;
        return;
    }

    std::vector<std::size_t> startV(start, start + *ndims);
    std::vector<std::size_t> countV(count, count + *ndims);

    try
    {
        adios2_set_selection(*variable, *ndims, startV.data(), countV.data());
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}
