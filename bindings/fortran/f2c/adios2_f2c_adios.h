/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_adios.h
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_FORTRAN_F2C_ADIOS2_F2C_ADIOS_H_
#define BINDINGS_FORTRAN_F2C_ADIOS2_F2C_ADIOS_H_

#include "adios2/ADIOSConfig.h"
#include <FC.h>
#include <adios2_c.h>

#ifdef ADIOS2_HAVE_MPI_F
#include <mpi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ADIOS2_HAVE_MPI_F
void FC_GLOBAL(adios2_init_f2c,
               ADIOS2_INIT_F2C)(adios2_adios **adios, MPI_Fint *comm,
                                const int *debug_mode, int *ierr);

void FC_GLOBAL(adios2_init_config_f2c,
               ADIOS2_INIT_CONFIG_F2C)(adios2_adios **adios,
                                       const char *config_file, MPI_Fint *comm,
                                       const int *debug_mode, int *ierr);
#else
void FC_GLOBAL(adios2_init_f2c, ADIOS2_INIT_F2C)(adios2_adios **adios,
                                                 const int *debug_mode,
                                                 int *ierr);

void FC_GLOBAL(adios2_init_config_f2c,
               ADIOS2_INIT_CONFIG_F2C)(adios2_adios **adios,
                                       const char *config_file,
                                       const int *debug_mode, int *ierr);
#endif

void FC_GLOBAL(adios2_declare_io_f2c,
               ADIOS2_DECLARE_IO_F2C)(adios2_io **io, adios2_adios **adios,
                                      const char *io_name, int *ierr);

void FC_GLOBAL(adios2_flush_all_f2c, ADIOS2_FLUSH_ALL_F2C)(adios2_adios **adios,
                                                           int *ierr);

void FC_GLOBAL(adios2_finalize_f2c, ADIOS2_FINALIZE_F2C)(adios2_adios **adios,
                                                         int *ierr);

#ifdef __cplusplus
}
#endif

#endif /* BINDINGS_FORTRAN_F2C_ADIOS2_F2C_ADIOS_H_ */
