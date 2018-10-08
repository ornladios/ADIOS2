/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_FILE.cpp
 *
 *  Created on: Feb 28, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_f2c_common.h"

#include <iostream>
#include <stdexcept> //std::invalid_argument

namespace
{

std::vector<std::size_t> adios2_Int64ToSizeTVector(const int64_t *dimensions,
                                                   const int size)
{

    if (dimensions == nullptr || size <= 0)
    {
        throw std::invalid_argument("ERROR: corrupted ndims or shape, "
                                    "start, count dimensions in Fortran ");
    }

    std::vector<std::size_t> output(size);

    for (int d = 0; d < size; ++d)
    {
        if (dimensions[d] < 0)
        {
            throw std::invalid_argument("ERROR: negative shape, start, or "
                                        "count dimension in Fortran\n");
        }
        output[d] = dimensions[d];
    }
    return output;
}
} // end empty namespace

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ADIOS2_HAVE_MPI_F

// these functions are not exposed in the public C API
extern adios2_FILE *adios2_fopen_glue(const char *name, const char *mode,
                                      MPI_Comm comm, const char *host_language);

extern adios2_FILE *adios2_fopen_config_glue(const char *name, const char *mode,
                                             MPI_Comm comm,
                                             const char *config_file,
                                             const char *io_in_config_file,
                                             const char *host_language);

void FC_GLOBAL(adios2_fopen_f2c,
               adios2_FOPEN_F2C)(adios2_FILE **fh, const char *name,
                                 const char *mode, int *comm, int *ierr)
{
    try
    {
        *fh = adios2_fopen_glue(name, mode, MPI_Comm_f2c(*comm), "Fortran");
        *ierr = static_cast<int>(adios2_error_none);
    }
    catch (...)
    {
        *ierr = static_cast<int>(adios2_error_exception);
    }
}

void FC_GLOBAL(adios2_fopen_config_f2c, adios2_FOPEN_CONFIG_F2C)(
    adios2_FILE **fh, const char *name, const char *mode, int *comm,
    const char *config_file, const char *io_in_config_file, int *ierr)
{
    try
    {
        *fh =
            adios2_fopen_config_glue(name, mode, MPI_Comm_f2c(*comm),
                                     config_file, io_in_config_file, "Fortran");
        *ierr = static_cast<int>(adios2_error_none);
    }
    catch (...)
    {
        *ierr = static_cast<int>(adios2_error_exception);
    }
}
#else
// these functions are not exposed in the public C API
extern adios2_FILE *adios2_fopen_glue(const char *name, const char *mode,
                                      const char *host_language);

extern adios2_FILE *adios2_fopen_config_glue(const char *name, const char *mode,
                                             const char *config_file,
                                             const char *io_in_config_file,
                                             const char *host_language);

void FC_GLOBAL(adios2_fopen_f2c, adios2_FOPEN_F2C)(adios2_FILE **fh,
                                                   const char *name,
                                                   const char *mode, int *ierr)
{
    try
    {
        *fh = adios2_fopen_glue(name, mode, "Fortran");
        *ierr = static_cast<int>(adios2_error_none);
    }
    catch (...)
    {
        *ierr = static_cast<int>(adios2_error_exception);
    }
}

void FC_GLOBAL(adios2_fopen_config_f2c, adios2_FOPEN_CONFIG_F2C)(
    adios2_FILE **fh, const char *name, const char *mode,
    const char *config_file, const char *io_in_config_file, int *ierr)
{
    try
    {
        *fh = adios2_fopen_config_glue(name, mode, config_file,
                                       io_in_config_file, "Fortran");
        *ierr = static_cast<int>(adios2_error_none);
    }
    catch (...)
    {
        *ierr = static_cast<int>(adios2_error_exception);
    }
}
#endif

void FC_GLOBAL(adios2_fwrite_value_f2c,
               adios2_FWRITE_VALUE_F2C)(adios2_FILE **fh, const char *name,
                                        const int *type, const void *data,
                                        const int *end_step, int *ierr)
{
    *ierr = adios2_fwrite(*fh, name, static_cast<adios2_type>(*type), data, 0,
                          nullptr, nullptr, nullptr,
                          static_cast<adios2_bool>(*end_step));
}

void FC_GLOBAL(adios2_fwrite_f2c,
               adios2_FWRITE_F2C)(adios2_FILE **fh, const char *name,
                                  const int *type, const void *data,
                                  const int *ndims, const int64_t *shape,
                                  const int64_t *start, const int64_t *count,
                                  const int *end_step, int *ierr)
{
    try
    {
        const std::vector<std::size_t> shapeV =
            adios2_Int64ToSizeTVector(shape, *ndims);
        const std::vector<std::size_t> startV =
            adios2_Int64ToSizeTVector(start, *ndims);
        const std::vector<std::size_t> countV =
            adios2_Int64ToSizeTVector(count, *ndims);

        *ierr = static_cast<int>(adios2_fwrite(
            *fh, name, static_cast<adios2_type>(*type), data,
            static_cast<size_t>(*ndims), shapeV.data(), startV.data(),
            countV.data(), static_cast<adios2_bool>(*end_step)));
    }
    catch (std::exception &e)
    {
        *ierr = static_cast<int>(adios2_error_exception);
    }
}

void FC_GLOBAL(adios2_fread_value_f2c,
               adios2_FREAD_VALUE_F2C)(adios2_FILE **fh, const char *name,
                                       const int *type, void *data,
                                       const int *end_step, int *ierr)
{
    try
    {
        if (*end_step != 0 && *end_step != 1)
        {
            throw std::invalid_argument(
                "ERROR: advance must be adios2_advance_yes(1) or "
                "adios2_advance_no(0), in call to adios2_fread");
        }

        *ierr = adios2_fread(*fh, name, static_cast<adios2_type>(*type), data,
                             0, nullptr, nullptr);
        if (*end_step == 1)
        {
            if (adios2_fgets(*fh, *fh) == nullptr)
            {
                *ierr = -1; // end of file
            }
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fread value variable " << std::string(name) << " "
                  << e.what() << "\n";
        *ierr = static_cast<int>(adios2_error_exception);
    }
}

void FC_GLOBAL(adios2_fread_value_step_f2c,
               adios2_FREAD_VALUE_step_F2C)(adios2_FILE **fh, const char *name,
                                            const int *type, void *data,
                                            const int64_t *step_selection_start,
                                            int *ierr)
{
    *ierr = static_cast<int>(adios2_fread_steps(
        *fh, name, static_cast<adios2_type>(*type), data, 0, nullptr, nullptr,
        static_cast<std::size_t>(*step_selection_start), 1));
}

void FC_GLOBAL(adios2_fread_f2c,
               adios2_FREAD_F2C)(adios2_FILE **fh, const char *name,
                                 const int *type, void *data, const int *ndims,
                                 const int64_t *start, const int64_t *count,
                                 const int *end_step, int *ierr)
{
    *ierr = 0;
    try
    {
        const std::vector<std::size_t> startV =
            adios2_Int64ToSizeTVector(start, *ndims);
        const std::vector<std::size_t> countV =
            adios2_Int64ToSizeTVector(count, *ndims);

        *ierr = adios2_fread(*fh, name, static_cast<adios2_type>(*type), data,
                             static_cast<size_t>(*ndims), startV.data(),
                             countV.data());
        if (*end_step == 1)
        {
            if (adios2_fgets(*fh, *fh) == nullptr)
            {
                *ierr = -1; // end of file
            }
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fread array variable " << std::string(name) << " "
                  << e.what() << "\n";
        *ierr = static_cast<int>(adios2_error_exception);
    }
}

void FC_GLOBAL(adios2_fread_steps_f2c, adios2_FREAD_STEPS_F2C)(
    adios2_FILE **fh, const char *name, const int *type, void *data,
    const int *ndims, const int64_t *start, const int64_t *count,
    const int64_t *step_start, const int64_t *step_count, int *ierr)
{
    try
    {
        const std::vector<std::size_t> startV =
            adios2_Int64ToSizeTVector(start, *ndims);
        const std::vector<std::size_t> countV =
            adios2_Int64ToSizeTVector(count, *ndims);

        *ierr = adios2_fread_steps(*fh, name, static_cast<adios2_type>(*type),
                                   data, static_cast<std::size_t>(*ndims),
                                   startV.data(), countV.data(),
                                   static_cast<std::size_t>(*step_start),
                                   static_cast<std::size_t>(*step_count));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fread variable " << std::string(name) << " "
                  << e.what() << "\n";
        *ierr = static_cast<int>(adios2_error_exception);
    }
}

void FC_GLOBAL(adios2_fclose_f2c, adios2_FCLOSE_F2C)(adios2_FILE **fh,
                                                     int *ierr)
{
    *ierr = adios2_fclose(*fh);
}

#ifdef __cplusplus
}
#endif
