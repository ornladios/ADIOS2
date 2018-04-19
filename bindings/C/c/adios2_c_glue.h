/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_glue.h : used by languages other than C, using the C-bindings API
 * (e.g. Fortran), not meant to be used by applications
 *
 *  Created on: Nov 3, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_C_C_ADIOS2_C_GLUE_H_
#define BINDINGS_C_C_ADIOS2_C_GLUE_H_

#include "adios2_c_FILE.h"
#include "adios2_c_adios.h"
#include "adios2_c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

adios2_adios *adios2_init_config_glue(const char *config_file,
                                      MPI_Comm mpi_comm,
                                      const adios2_debug_mode debug_mode,
                                      const char *host_language);

/**
 * Create an ADIOS struct pointer in MPI application.
 * @param mpi_comm MPI communicator from application for ADIOS scope
 * @param debug_mode adios2_debug_mode_on or adios2_debug_mode_off
 * @return valid ADIOS* handler
 */
adios2_adios *adios2_init_glue(MPI_Comm mpi_comm,
                               const adios2_debug_mode debug_mode,
                               const char *host_language);

/**
 * Create an ADIOS struct pointer handler using a runtime config file in serial
 * nonMPI
 * application.
 * @param config_file runtime configuration file, XML format, future: JSON
 * @param debug_mode adios2_debug_mode_on or adios2_debug_mode_off
 * @return valid ADIOS* handler
 */
adios2_adios *adios2_init_config_nompi_glue(const char *config_file,
                                            const adios2_debug_mode debug_mode,
                                            const char *host_language);

/**
 * Create an ADIOS struct pointer handler in serial nonMPI application.
 * @param debug_mode adios2_debug_mode_on or adios2_debug_mode_off
 * @param host_language language calling C glue interface
 * @return valid ADIOS* handler
 */
adios2_adios *adios2_init_nompi_glue(const adios2_debug_mode debug_mode,
                                     const char *host_language);

/**
 * Higher-level API stream glue code (called from other languages)
 * @param name unique name of the bpstream
 * @param mode open mode: write, read, append
 * @param mpi_comm MPI communicator
 * @param host_language language calling C glue interface
 * @return handler to high-level api adios2_stream
 */
adios2_FILE *adios2_fopen_glue(const char *name, const adios2_mode mode,
                               MPI_Comm mpi_comm, const char *host_language);

adios2_FILE *adios2_fopen_config_glue(const char *name, const adios2_mode mode,
                                      MPI_Comm mpi_comm,
                                      const char *config_file,
                                      const char *io_in_config_file,
                                      const char *host_language);

adios2_FILE *adios2_fopen_nompi_glue(const char *name, const adios2_mode mode,
                                     const char *host_language);

adios2_FILE *adios2_fopen_config_nompi_glue(const char *name,
                                            const adios2_mode mode,
                                            const char *config_file,
                                            const char *io_in_config_file,
                                            const char *host_language);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* BINDINGS_C_C_ADIOS2_C_GLUE_H_ */
