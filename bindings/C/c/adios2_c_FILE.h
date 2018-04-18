/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_FILE.h : high-level API functions
 *
 *  Created on: Jan 8, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_C_C_ADIOS2_C_FILE_H_
#define BINDINGS_C_C_ADIOS2_C_FILE_H_

#include "adios2/ADIOSMPICommOnly.h"

#include "adios2_c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

adios2_FILE *adios2_fopen(const char *name, const adios2_mode mode,
                          MPI_Comm comm);

adios2_FILE *adios2_fopen_nompi(const char *name, const adios2_mode mode);

adios2_FILE *adios2_fopen_config(const char *name, const adios2_mode mode,
                                 MPI_Comm comm, const char *config_file,
                                 const char *io_in_config_file);

adios2_FILE *adios2_fopen_config_nompi(const char *name, const adios2_mode mode,
                                       const char *config_file,
                                       const char *io_in_config_file);

void adios2_fwrite(adios2_FILE *stream, const char *name,
                   const adios2_type type, const void *data, const size_t ndims,
                   const size_t *shape, const size_t *start,
                   const size_t *count, const int end_step);

void adios2_fread(adios2_FILE *stream, const char *name, const adios2_type type,
                  void *data, const size_t ndims, const size_t *selection_start,
                  const size_t *selection_count, const int end_step);

void adios2_fread_steps(adios2_FILE *stream, const char *name,
                        const adios2_type type, void *data, const size_t ndims,
                        const size_t *selection_start,
                        const size_t *selection_count,
                        const size_t step_selection_start,
                        const size_t step_selection_count);

void adios2_fclose(adios2_FILE *stream);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* BINDINGS_C_C_ADIOS2_C_FILE_H_ */
