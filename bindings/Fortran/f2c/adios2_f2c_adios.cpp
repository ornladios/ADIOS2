/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_adios.cpp
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_f2c_adios.h"

#include <iostream> //std::cerr
#include <stdexcept>

#ifdef ADIOS2_HAVE_MPI_F
void FC_GLOBAL(adios2_init_f2c,
               ADIOS2_INIT_F2C)(adios2_adios **adios, MPI_Fint *comm,
                                const int *debug_mode, int *ierr)
{
    FC_GLOBAL(adios2_init_config_f2c, ADIOS2_INIT_CONFIG_F2C)
    (adios, "", comm, debug_mode, ierr);
}

void FC_GLOBAL(adios2_init_config_f2c,
               ADIOS2_INIT_CONFIG_F2C)(adios2_adios **adios,
                                       const char *config_file, MPI_Fint *comm,
                                       const int *debug_mode, int *ierr)
{
    *ierr = 0;
    try
    {
        *adios = adios2_init_config_glue(
            config_file, MPI_Comm_f2c(*comm),
            static_cast<adios2_debug_mode>(*debug_mode), "Fortran");
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 init_config: " << e.what() << "\n";
        *ierr = -1;
    }
}
#else
void FC_GLOBAL(adios2_init_f2c, ADIOS2_INIT_F2C)(adios2_adios **adios,
                                                 const int *debug_mode,
                                                 int *ierr)
{
    FC_GLOBAL(adios2_init_config_f2c, ADIOS2_INIT_CONFIG_F2C)
    (adios, "", debug_mode, ierr);
}

void FC_GLOBAL(adios2_init_config_f2c,
               ADIOS2_INIT_CONFIG_F2C)(adios2_adios **adios,
                                       const char *config_file,
                                       const int *debug_mode, int *ierr)
{
    *ierr = 0;
    try
    {
        *adios = adios2_init_config_nompi_glue(
            config_file, static_cast<adios2_debug_mode>(*debug_mode),
            "Fortran");
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 init_config: " << e.what() << "\n";
        *ierr = -1;
    }
}
#endif

void FC_GLOBAL(adios2_declare_io_f2c,
               ADIOS2_DECLARE_IO_F2C)(adios2_io **io, adios2_adios **adios,
                                      const char *io_name, int *ierr)
{
    *ierr = 0;

    try
    {
        *io = adios2_declare_io(*adios, io_name);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 declare_io: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_at_io_f2c,
               ADIOS2_at_IO_F2C)(adios2_io **io, adios2_adios **adios,
                                 const char *io_name, int *ierr)
{
    *ierr = 0;

    try
    {
        *io = adios2_at_io(*adios, io_name);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 at_io: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_define_operator_f2c,
               ADIOS2_DEFINE_OPERATOR_F2C)(adios2_operator **op,
                                           adios2_adios **adios,
                                           const char *op_name,
                                           const char *op_type, int *ierr)
{
    *ierr = 0;
    try
    {
        *op = adios2_define_operator(*adios, op_name, op_type);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 define_operator: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_inquire_operator_f2c,
               ADIOS2_INQUIRE_OPERATOR_F2C)(adios2_operator **op,
                                            adios2_adios **adios,
                                            const char *op_name, int *ierr)
{
    *ierr = 0;
    try
    {
        *op = adios2_inquire_operator(*adios, op_name);
        if (*op == nullptr)
        {
            *ierr = 1;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 inquire_operator: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_flush_all_f2c, ADIOS2_FLUSH_ALL_F2C)(adios2_adios **adios,
                                                           int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_flush_all(*adios);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 flush_all: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_finalize_f2c, ADIOS2_FINALIZE_F2C)(adios2_adios **adios,
                                                         int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_finalize(*adios);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 finalize: " << e.what() << "\n";
        *ierr = -1;
    }
}
