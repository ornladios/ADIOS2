/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_adios.cpp
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_f2c_common.h"

#include "adios2/ADIOSMPI.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ADIOS2_HAVE_MPI_F

// this function is not exposed in the public APIs
extern adios2_adios *adios2_init_config_glue(const char *config_file,
                                             MPI_Comm comm,
                                             const adios2_debug_mode debug_mode,
                                             const char *host_language);

void FC_GLOBAL(adios2_init_config_f2c,
               ADIOS2_INIT_CONFIG_F2C)(adios2_adios **adios,
                                       const char *config_file, MPI_Fint *comm,
                                       const int *debug_mode, int *ierr)
{
    *adios = adios2_init_config_glue(
        config_file, MPI_Comm_f2c(*comm),
        static_cast<adios2_debug_mode>(*debug_mode), "Fortran");
    *ierr = (*adios == NULL) ? static_cast<int>(adios2_error_exception)
                             : static_cast<int>(adios2_error_none);
}

void FC_GLOBAL(adios2_init_f2c,
               ADIOS2_INIT_F2C)(adios2_adios **adios, MPI_Fint *comm,
                                const int *debug_mode, int *ierr)
{
    FC_GLOBAL(adios2_init_config_f2c, ADIOS2_INIT_CONFIG_F2C)
    (adios, "", comm, debug_mode, ierr);
}
#else

// this function is not exposed in the public APIs
extern adios2_adios *adios2_init_config_glue(const char *config_file,
                                             const adios2_debug_mode debug_mode,
                                             const char *host_language);

void FC_GLOBAL(adios2_init_config_f2c,
               ADIOS2_INIT_CONFIG_F2C)(adios2_adios **adios,
                                       const char *config_file,
                                       const int *debug_mode, int *ierr)
{
    *adios = adios2_init_config_glue(
        config_file, static_cast<adios2_debug_mode>(*debug_mode), "Fortran");
    *ierr = (*adios == NULL) ? static_cast<int>(adios2_error_exception)
                             : static_cast<int>(adios2_error_none);
}

void FC_GLOBAL(adios2_init_f2c, ADIOS2_INIT_F2C)(adios2_adios **adios,
                                                 const int *debug_mode,
                                                 int *ierr)
{
    FC_GLOBAL(adios2_init_config_f2c, ADIOS2_INIT_CONFIG_F2C)
    (adios, "", debug_mode, ierr);
}
#endif

void FC_GLOBAL(adios2_declare_io_f2c,
               ADIOS2_DECLARE_IO_F2C)(adios2_io **io, adios2_adios **adios,
                                      const char *name, int *ierr)
{

    *io = adios2_declare_io(*adios, name);
    *ierr = (*io == NULL) ? static_cast<int>(adios2_error_exception)
                          : static_cast<int>(adios2_error_none);
}

void FC_GLOBAL(adios2_at_io_f2c, ADIOS2_at_IO_F2C)(adios2_io **io,
                                                   adios2_adios **adios,
                                                   const char *name, int *ierr)
{
    *io = adios2_at_io(*adios, name);
    *ierr = (*io == NULL) ? static_cast<int>(adios2_error_exception)
                          : static_cast<int>(adios2_error_none);
}

void FC_GLOBAL(adios2_define_operator_f2c,
               ADIOS2_DEFINE_OPERATOR_F2C)(adios2_operator **op,
                                           adios2_adios **adios,
                                           const char *op_name,
                                           const char *op_type, int *ierr)
{
    *op = adios2_define_operator(*adios, op_name, op_type);
    *ierr = (*op == NULL) ? static_cast<int>(adios2_error_exception)
                          : static_cast<int>(adios2_error_none);
}

void FC_GLOBAL(adios2_inquire_operator_f2c,
               ADIOS2_INQUIRE_OPERATOR_F2C)(adios2_operator **op,
                                            adios2_adios **adios,
                                            const char *op_name, int *ierr)
{
    *op = adios2_inquire_operator(*adios, op_name);
    *ierr = (*op == NULL) ? static_cast<int>(adios2_error_exception)
                          : static_cast<int>(adios2_error_none);
}

void FC_GLOBAL(adios2_flush_all_f2c, ADIOS2_FLUSH_ALL_F2C)(adios2_adios **adios,
                                                           int *ierr)
{
    *ierr = static_cast<int>(adios2_flush_all(*adios));
}

void FC_GLOBAL(adios2_remove_io_f2c,
               ADIOS2_REMOVE_IO_F2C)(int *result, adios2_adios **adios,
                                     const char *name, int *ierr)
{
    adios2_bool resultC;
    *ierr = static_cast<int>(adios2_remove_io(&resultC, *adios, name));
    if (*ierr == static_cast<int>(adios2_error_none))
    {
        *result = (resultC == adios2_true) ? 1 : 0;
    }
}

void FC_GLOBAL(adios2_remove_all_ios_f2c,
               ADIOS2_REMOVE_ALL_IOS_F2C)(adios2_adios **adios, int *ierr)
{
    *ierr = static_cast<int>(adios2_remove_all_ios(*adios));
}

void FC_GLOBAL(adios2_finalize_f2c, ADIOS2_FINALIZE_F2C)(adios2_adios **adios,
                                                         int *ierr)
{
    *ierr = static_cast<int>(adios2_finalize(*adios));
}

#ifdef __cplusplus
}
#endif
