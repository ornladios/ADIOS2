/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f.cpp :  C-header glue code implmentation for functions called from
 * Fortran modules
 *
 *  Created on: Aug 14, 2017
 *      Author: William F Godoy
 */

#include "adios2_f2c.h"

#include <stdexcept>
#include <type_traits> //std::static_assert
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ADIOS2_HAVE_MPI_F
void adios2_init_f2c_(adios2_ADIOS **adios, MPI_Fint *comm,
                      const int *debug_mode, int *ierr)
{
    adios2_init_config_f2c_(adios, "", comm, debug_mode, ierr);
}

void adios2_init_config_f2c_(adios2_ADIOS **adios, const char *config_file,
                             MPI_Fint *comm, const int *debug_mode, int *ierr)
{
    *ierr = 0;
    try
    {
        *adios =
            adios2_init_config(config_file, MPI_Comm_f2c(*comm),
                               static_cast<adios2_debug_mode>(*debug_mode));
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}
#else
void adios2_init_f2c_(adios2_ADIOS **adios, const int *debug_mode, int *ierr)
{
    adios2_init_config_f2c_(adios, "", debug_mode, ierr);
}

void adios2_init_config_f2c_(adios2_ADIOS **adios, const char *config_file,
                             const int *debug_mode, int *ierr)
{
    *ierr = 0;
    try
    {
        *adios = adios2_init_config(
            config_file, static_cast<adios2_debug_mode>(*debug_mode));
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}
#endif

void adios2_declare_io_f2c_(adios2_IO **io, adios2_ADIOS **adios,
                            const char *io_name, int *ierr)
{
    *ierr = 0;

    try
    {
        *io = adios2_declare_io(*adios, io_name);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void adios2_set_param_f2c_(adios2_IO **io, const char *key, const char *value,
                           int *ierr)
{
    *ierr = 0;

    try
    {
        adios2_set_param(*io, key, value);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void adios2_add_transport_f2c_(int *transport_index, adios2_IO **io,
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

void adios2_set_transport_param_f2c_(adios2_IO **io, const int *transport_index,
                                     const char *key, const char *value,
                                     int *ierr)
{
    *ierr = 0;

    try
    {
        adios2_set_transport_param(
            *io, static_cast<unsigned int>(*transport_index), key, value);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void adios2_define_variable_f2c_(adios2_Variable **variable, adios2_IO **io,
                                 const char *variable_name, const int *type,
                                 const int *ndims, const int *shape,
                                 const int *start, const int *count,
                                 const int *constant_dims, int *ierr)
{
    auto lf_IntToSizeT = [](const int *ndims,
                            const int *dims) -> std::vector<std::size_t> {

        std::vector<std::size_t> vecSizeT(*ndims);
        for (unsigned int dim = 0; dim < *ndims; ++dim)
        {
            vecSizeT[dim] = dims[dim];
        }
        return vecSizeT;
    };

    *ierr = 0;

    std::vector<std::size_t> shapeV, startV, countV;
    if (shape != NULL)
    {
        shapeV = lf_IntToSizeT(ndims, shape);
    }

    if (start != NULL)
    {
        startV = lf_IntToSizeT(ndims, start);
    }

    if (count != NULL)
    {
        countV = lf_IntToSizeT(ndims, count);
    }

    try
    {
        *variable = adios2_define_variable(
            *io, variable_name, static_cast<adios2_type>(*type), *ndims,
            shapeV.data(), startV.data(), countV.data(),
            static_cast<adios2_constant_dims>(*constant_dims));
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void adios2_open_f2c_(adios2_Engine **engine, adios2_IO **io, const char *name,
                      const int *open_mode, int *ierr)
{
    *ierr = 0;
    try
    {
        *engine =
            adios2_open(*io, name, static_cast<adios2_open_mode>(*open_mode));
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

#ifdef ADIOS2_HAVE_MPI_F
void adios2_open_new_comm_f2c_(adios2_Engine **engine, adios2_IO **io,
                               const char *name, const int *open_mode,
                               MPI_Fint *comm, int *ierr)
{
    *ierr = 0;
    try
    {
        *engine = adios2_open_new_comm(
            *io, name, static_cast<adios2_open_mode>(*open_mode),
            MPI_Comm_f2c(*comm));
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}
#endif

void adios2_write_f2c_(adios2_Engine **engine, adios2_Variable **variable,
                       const void *values, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_write(*engine, *variable, values);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void adios2_advance_f2c_(adios2_Engine **engine, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_advance(*engine);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void adios2_close_f2c_(adios2_Engine **engine, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_close(*engine);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void adios2_finalize_f2c_(adios2_ADIOS **adios, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_finalize(*adios);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

#ifdef __cplusplus
}
#endif
