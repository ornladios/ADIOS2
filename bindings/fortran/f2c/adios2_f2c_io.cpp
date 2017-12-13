/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_io.cpp
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_f2c_io.h"

#include <cstddef> //std::size_t
#include <stdexcept>
#include <vector>

void FC_GLOBAL(adios2_set_parameter_f2c,
               ADIOS2_SET_PARAMETER_F2C)(adios2_IO **io, const char *key,
                                         const char *value, int *ierr)
{
    *ierr = 0;

    try
    {
        adios2_set_parameter(*io, key, value);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_add_transport_f2c,
               ADIOS2_ADD_TRANSPORT_F2C)(int *transport_index, adios2_IO **io,
                                         const char *transport_type, int *ierr)
{
    *ierr = 0;
    *transport_index = -1;

    try
    {
        *transport_index =
            static_cast<int>(adios2_add_transport(*io, transport_type));
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }

    if (*transport_index == -1)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_set_transport_parameter_f2c,
               ADIOS2_SET_TRANSPORT_PARAMETER_F2C)(adios2_IO **io,
                                                   const int *transport_index,
                                                   const char *key,
                                                   const char *value, int *ierr)
{
    *ierr = 0;

    try
    {
        adios2_set_transport_parameter(
            *io, static_cast<unsigned int>(*transport_index), key, value);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_define_variable_f2c, ADIOS2_DEFINE_VARIABLE_F2C)(
    adios2_Variable **variable, adios2_IO **io, const char *variable_name,
    const int *type, const int *ndims, const int *shape, const int *start,
    const int *count, const int *constant_dims, void *data, int *ierr)
{
    auto lf_IntToSizeT = [](const int *dimensions, const int size,
                            std::vector<std::size_t> &output) {

        if (dimensions == nullptr || size == 0)
        {
            return;
        }

        output.resize(size);

        for (unsigned int d = 0; d < size; ++d)
        {
            if (dimensions[d] < 0)
            {
                throw std::invalid_argument("ERROR: negative shape, start, or "
                                            "count dimension in Fortran\n");
            }
            output[d] = dimensions[d];
        }
    };

    *ierr = 0;

    std::vector<std::size_t> shapeV, startV, countV;
    lf_IntToSizeT(shape, *ndims, shapeV);
    lf_IntToSizeT(start, *ndims, startV);
    lf_IntToSizeT(count, *ndims, countV);

    try
    {
        *variable = adios2_define_variable(
            *io, variable_name, static_cast<adios2_type>(*type), *ndims,
            shapeV.data(), startV.data(), countV.data(),
            static_cast<adios2_constant_dims>(*constant_dims), data);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_inquire_variable_f2c,
               ADIOS2_INQUIRE_VARIABLE_F2C)(adios2_Variable **variable,
                                            adios2_IO **io,
                                            const char *variable_name,
                                            int *ierr)
{
    *ierr = 0;
    try
    {
        *variable = adios2_inquire_variable(*io, variable_name);
        if (*variable == nullptr)
        {
            *ierr = 2;
        }
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_open_f2c,
               ADIOS2_OPEN_F2C)(adios2_Engine **engine, adios2_IO **io,
                                const char *name, const int *open_mode,
                                int *ierr)
{
    *ierr = 0;
    try
    {
        *engine = adios2_open(*io, name, static_cast<adios2_mode>(*open_mode));
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

#ifdef ADIOS2_HAVE_MPI_F
void FC_GLOBAL(adios2_open_new_comm_f2c,
               ADIOS2_OPEN_NEW_COMM_F2C)(adios2_Engine **engine, adios2_IO **io,
                                         const char *name, const int *open_mode,
                                         MPI_Fint *comm, int *ierr)
{
    *ierr = 0;
    try
    {
        *engine = adios2_open_new_comm(*io, name,
                                       static_cast<adios2_mode>(*open_mode),
                                       MPI_Comm_f2c(*comm));
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}
#endif
