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

namespace
{

void adios2_Int64ToSizeTVector(const int64_t *dimensions, const int size,
                               std::vector<std::size_t> &output)
{

    if (dimensions == nullptr || size <= 0)
    {
        throw std::invalid_argument("ERROR: corrupted ndims or shape, "
                                    "start, count dimensions in Fortran ");
    }

    output.resize(size);

    for (int d = 0; d < size; ++d)
    {
        if (dimensions[d] < 0)
        {
            throw std::invalid_argument("ERROR: negative shape, start, or "
                                        "count dimension in Fortran\n");
        }
        output[d] = dimensions[d];
    }
}
} // end empty namespace

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ADIOS2_HAVE_MPI_F
void FC_GLOBAL(adios2_fopen_f2c,
               adios2_FOPEN_F2C)(adios2_FILE **fh, const char *name,
                                 const int *mode, int *comm, int *ierr)
{
    *ierr = 0;
    try
    {
        *fh = adios2_fopen_glue(name, static_cast<adios2_mode>(*mode),
                                MPI_Comm_f2c(*comm), "Fortran");
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fopen with name " << std::string(name) << " "
                  << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_fopen_config_f2c, adios2_FOPEN_CONFIG_F2C)(
    adios2_FILE **fh, const char *name, const int *mode, int *comm,
    const char *config_file, const char *io_in_config_file, int *ierr)
{
    *ierr = 0;
    try
    {
        *fh = adios2_fopen_config_glue(name, static_cast<adios2_mode>(*mode),
                                       MPI_Comm_f2c(*comm), config_file,
                                       io_in_config_file, "Fortran");
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fopen with name " << std::string(name) << " "
                  << e.what() << "\n";
        *ierr = -1;
    }
}
#else
void FC_GLOBAL(adios2_fopen_f2c, adios2_FOPEN_F2C)(adios2_FILE **fh,
                                                   const char *name,
                                                   const int *mode, int *ierr)
{
    *ierr = 0;
    try
    {
        *fh = adios2_fopen_nompi_glue(name, static_cast<adios2_mode>(*mode),
                                      "Fortran");
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fopen with name " << std::string(name) << " "
                  << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_fopen_config_f2c, adios2_FOPEN_CONFIG_F2C)(
    adios2_FILE **fh, const char *name, const int *mode,
    const char *config_file, const char *io_in_config_file, int *ierr)
{
    *ierr = 0;
    try
    {
        *fh = adios2_fopen_config_nompi_glue(
            name, static_cast<adios2_mode>(*mode), config_file,
            io_in_config_file, "Fortran");
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fopen with name " << std::string(name) << " "
                  << e.what() << "\n";
        *ierr = -1;
    }
}
#endif

void FC_GLOBAL(adios2_fwrite_value_f2c,
               adios2_FWRITE_VALUE_F2C)(adios2_FILE **fh, const char *name,
                                        const int *type, const void *data,
                                        const int *end_step, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_fwrite(*fh, name, static_cast<adios2_type>(*type), data, 0,
                      nullptr, nullptr, nullptr, *end_step);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fwrite value variable: " << std::string(name)
                  << " " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_fwrite_f2c,
               adios2_FWRITE_F2C)(adios2_FILE **fh, const char *name,
                                  const int *type, const void *data,
                                  const int *ndims, const int64_t *shape,
                                  const int64_t *start, const int64_t *count,
                                  const int *end_step, int *ierr)
{
    *ierr = 0;

    try
    {
        std::vector<std::size_t> shapeV, startV, countV;
        adios2_Int64ToSizeTVector(shape, *ndims, shapeV);
        adios2_Int64ToSizeTVector(start, *ndims, startV);
        adios2_Int64ToSizeTVector(count, *ndims, countV);

        adios2_fwrite(*fh, name, static_cast<adios2_type>(*type), data,
                      static_cast<size_t>(*ndims), shapeV.data(), startV.data(),
                      countV.data(), *end_step);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fwrite array variable: " << std::string(name)
                  << " " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_fread_value_f2c,
               adios2_FREAD_VALUE_F2C)(adios2_FILE **fh, const char *name,
                                       const int *type, void *data,
                                       const int *end_step, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_fread(*fh, name, static_cast<adios2_type>(*type), data, 0,
                     nullptr, nullptr);
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
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_fread_value_step_f2c,
               adios2_FREAD_VALUE_step_F2C)(adios2_FILE **fh, const char *name,
                                            const int *type, void *data,
                                            const int64_t *step_selection_start,
                                            int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_fread_steps(*fh, name, static_cast<adios2_type>(*type), data, 0,
                           nullptr, nullptr,
                           static_cast<std::size_t>(*step_selection_start), 1);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fread value variable " << std::string(name)
                  << " and step " << step_selection_start << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_fread_f2c,
               adios2_FREAD_F2C)(adios2_FILE **fh, const char *name,
                                 const int *type, void *data, const int *ndims,
                                 const int64_t *selection_start,
                                 const int64_t *selection_count,
                                 const int *end_step, int *ierr)
{
    *ierr = 0;
    try
    {
        std::vector<std::size_t> selectionStartV, selectionCountV;
        adios2_Int64ToSizeTVector(selection_start, *ndims, selectionStartV);
        adios2_Int64ToSizeTVector(selection_count, *ndims, selectionCountV);

        adios2_fread(*fh, name, static_cast<adios2_type>(*type), data,
                     static_cast<size_t>(*ndims), selectionStartV.data(),
                     selectionCountV.data());
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
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_fread_steps_f2c, adios2_FREAD_STEPS_F2C)(
    adios2_FILE **fh, const char *name, const int *type, void *data,
    const int *ndims, const int64_t *selection_start,
    const int64_t *selection_count, const int64_t *step_selection_start,
    const int64_t *step_selection_count, int *ierr)
{

    *ierr = 0;
    try
    {
        std::vector<std::size_t> selectionStartV, selectionCountV;
        adios2_Int64ToSizeTVector(selection_start, *ndims, selectionStartV);
        adios2_Int64ToSizeTVector(selection_count, *ndims, selectionCountV);

        adios2_fread_steps(*fh, name, static_cast<adios2_type>(*type), data,
                           static_cast<std::size_t>(*ndims),
                           selectionStartV.data(), selectionCountV.data(),
                           static_cast<std::size_t>(*step_selection_start),
                           static_cast<std::size_t>(*step_selection_count));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fread variable " << std::string(name) << " "
                  << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_fclose_f2c, adios2_FCLOSE_F2C)(adios2_FILE **fh,
                                                     int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_fclose(*fh);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 fclose " << e.what() << "\n";
        *ierr = -1;
    }
}

#ifdef __cplusplus
}
#endif
