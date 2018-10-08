/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_FILE.h : high-level API functions
 *
 *  Created on: Jan 8, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_C_C_ADIOS2_C_FILE_H_
#define ADIOS2_BINDINGS_C_C_ADIOS2_C_FILE_H_

#include "adios2/ADIOSConfig.h"

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

#include "adios2_c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ADIOS2_HAVE_MPI
/**
 * User-level convenience API based on stdio.h FILE
 * @param name input stream name
 * @param mode
 * @param comm
 * @return
 */
adios2_FILE *adios2_fopen(const char *name, const char *mode, MPI_Comm comm);

adios2_FILE *adios2_fopen_config(const char *name, const char *mode,
                                 MPI_Comm comm, const char *config_file,
                                 const char *io_in_config_file);
#else
adios2_FILE *adios2_fopen(const char *name, const char *mode);

adios2_FILE *adios2_fopen_config(const char *name, const char *mode,
                                 const char *config_file,
                                 const char *io_in_config_file);
#endif

adios2_error adios2_fwrite(adios2_FILE *stream, const char *name,
                           const adios2_type type, const void *data,
                           const size_t ndims, const size_t *shape,
                           const size_t *start, const size_t *count,
                           const adios2_bool end_step);

adios2_step *adios2_fgets(adios2_step *step, adios2_FILE *stream);

adios2_error adios2_fread(adios2_FILE *stream, const char *name,
                          const adios2_type type, void *data,
                          const size_t ndims, const size_t *selection_start,
                          const size_t *selection_count);

adios2_error adios2_fread_steps(adios2_FILE *stream, const char *name,
                                const adios2_type type, void *data,
                                const size_t ndims,
                                const size_t *selection_start,
                                const size_t *selection_count,
                                const size_t step_selection_start,
                                const size_t step_selection_count);

adios2_error adios2_fclose(adios2_FILE *stream);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_C_ADIOS2_C_FILE_H_ */
