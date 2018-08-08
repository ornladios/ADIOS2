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

#include <cstddef>  //std::size_t
#include <iostream> //std::cerr
#include <stdexcept>
#include <vector>

void FC_GLOBAL(adios2_variable_name_f2c,
               ADIOS2_VARIABLE_NAME_F2C)(const adios2_variable **variable,
                                         char name[4096], int *length,
                                         int *ierr)
{
    *ierr = 0;
    try
    {
        std::size_t lengthC = 0;
        const char *nameC = adios2_variable_name(*variable, &lengthC);

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
        std::cerr << "ADIOS2 variable_name: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_variable_type_f2c,
               ADIOS2_VARIABLE_TYPE_F2C)(const adios2_variable **variable,
                                         int *type, int *ierr)
{
    *ierr = 0;
    try
    {
        *type = static_cast<int>(adios2_variable_type(*variable));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 variable_type: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_variable_ndims_f2c,
               ADIOS2_VARIABLE_NDIMS_F2C)(const adios2_variable **variable,
                                          int *ndims, int *ierr)
{
    *ierr = 0;
    try
    {
        *ndims = static_cast<int>(adios2_variable_ndims(*variable));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 variable_ndims: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_variable_shape_f2c,
               ADIOS2_VARIABLE_SHAPE_F2C)(const adios2_variable **variable,
                                          int64_t *shape, int *ierr)
{
    *ierr = 0;
    try
    {
        const size_t ndims = adios2_variable_ndims(*variable);
        const size_t *shapeC = adios2_variable_shape(*variable);

        for (auto d = 0; d < ndims; ++d)
        {
            shape[d] = static_cast<int64_t>(shapeC[d]);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 variable_shape: " << e.what() << "\n";
        *ierr = -1;
    }
}

void
    FC_GLOBAL(adios2_variable_steps_start_f2c,
              ADIOS2_VARIABLE_STEPS_START_F2C)(const adios2_variable **variable,
                                               int64_t *steps_start, int *ierr)
{
    *ierr = 0;
    try
    {
        *steps_start =
            static_cast<int64_t>(adios2_variable_steps_start(*variable));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 variable_steps_start: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_variable_steps_f2c,
               adios2_variable_STEPS_F2C)(const adios2_variable **variable,
                                          int64_t *steps_count, int *ierr)
{
    *ierr = 0;
    try
    {
        *steps_count = static_cast<int64_t>(adios2_variable_steps(*variable));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 variable_steps: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_set_shape_f2c,
               ADIOS2_SET_SHAPE_F2C)(adios2_variable **variable,
                                     const int *ndims, const int64_t *shape,
                                     int *ierr)
{
    *ierr = 0;
    try
    {
        if (shape == nullptr || ndims == nullptr)
        {
            throw std::invalid_argument(
                "ERROR: either shape_dims or ndims is a null pointer, in call "
                "to adios2_set_selection\n");
        }
        std::vector<std::size_t> shapeV(shape, shape + *ndims);
        adios2_set_shape(*variable, shapeV.size(), shapeV.data());
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 set_shape: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_set_selection_f2c,
               ADIOS2_SET_SELECTION_F2C)(adios2_variable **variable,
                                         const int *ndims, const int64_t *start,
                                         const int64_t *count, int *ierr)
{
    auto lf_IntToSizeT = [](const int64_t *dimensions, const int size,
                            std::vector<std::size_t> &output) {

        output.resize(size);

        for (unsigned int d = 0; d < size; ++d)
        {
            output[d] = dimensions[d];
        }

    };

    *ierr = 0;
    try
    {
        if (start == nullptr || count == nullptr || ndims == nullptr)
        {
            throw std::invalid_argument("ERROR: either start_dims, count_dims "
                                        "or ndims is a null pointer, in call "
                                        "to adios2_set_selection\n");
        }
        std::vector<std::size_t> startV, countV;
        lf_IntToSizeT(start, *ndims, startV);
        lf_IntToSizeT(count, *ndims, countV);
        adios2_set_selection(*variable, *ndims, startV.data(), countV.data());
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 set_selection: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_set_step_selection_f2c,
               ADIOS2_SET_STEP_SELECTION_F2C)(adios2_variable **variable,
                                              const int64_t *step_start,
                                              const int64_t *step_count,
                                              int *ierr)
{
    *ierr = 0;

    try
    {
        if (step_start == nullptr || step_count == nullptr)
        {
            throw std::invalid_argument(
                "ERROR: either step_start or step_count "
                "are null pointers, in call to "
                "adios2_set_step_selection\n");
        }

        if (step_start[0] < 0)
        {
            throw std::invalid_argument("ERROR: negative step_start in call to "
                                        "adios2_set_step_selection\n");
        }

        if (step_count[0] < 0)
        {
            throw std::invalid_argument("ERROR: negative step_count in call to "
                                        "adios2_set_step_selection\n");
        }

        const std::size_t stepStart = static_cast<std::size_t>(*step_start);
        const std::size_t stepCount = static_cast<std::size_t>(*step_count);
        adios2_set_step_selection(*variable, stepStart, stepCount);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 set_step_selection: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_add_operation_f2c,
               ADIOS2_ADD_OPERATION_F2C)(int *operation_id,
                                         adios2_variable **variable,
                                         adios2_operator **op, const char *key,
                                         const char *value, int *ierr)
{
    *ierr = 0;
    try
    {
        *operation_id =
            static_cast<int>(adios2_add_operation(*variable, *op, key, value));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 add_operation: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_set_operation_parameter_f2c,
               ADIOS2_SET_OPERATION_PARAMETER_F2C)(adios2_variable **variable,
                                                   const int *operation_id,
                                                   const char *key,
                                                   const char *value, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_set_operation_parameter(
            *variable, static_cast<std::size_t>(*operation_id), key, value);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 set_operation_parameter: " << e.what() << "\n";
        *ierr = -1;
    }
}
