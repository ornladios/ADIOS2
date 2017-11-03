/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_glue.h : used by languages, other than C, using the C-bindings API
 * (e.g. Fortran), not meant to be for applications
 *
 *  Created on: Nov 3, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_C_ADIOS2_ADIOS2_C_GLUE_H_
#define BINDINGS_C_ADIOS2_ADIOS2_C_GLUE_H_

#include "adios2/adios2_c_types.h"
#include "adios2_c.h"

#ifdef __cplusplus
extern "C" {
#endif

adios2_ADIOS *adios2_init_config_glue(const char *config_file,
                                      MPI_Comm mpi_comm,
                                      const adios2_debug_mode debug_mode,
                                      const char *host_language);

/**
 * Create an ADIOS struct pointer in MPI application.
 * @param mpi_comm MPI communicator from application for ADIOS scope
 * @param debug_mode adios2_debug_mode_on or adios2_debug_mode_off
 * @return valid ADIOS* handler
 */
adios2_ADIOS *adios2_init_glue(MPI_Comm mpi_comm,
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
adios2_ADIOS *adios2_init_config_nompi_glue(const char *config_file,
                                            const adios2_debug_mode debug_mode,
                                            const char *host_language);

/**
 * Create an ADIOS struct pointer handler in serial nonMPI application.
 * @param debug_mode adios2_debug_mode_on or adios2_debug_mode_off
 * @return valid ADIOS* handler
 */
adios2_ADIOS *adios2_init_nompi_glue(const adios2_debug_mode debug_mode,
                                     const char *host_language);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* BINDINGS_C_ADIOS2_ADIOS2_C_GLUE_H_ */
