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
 * Open and adios2 stream adios2_FILE*, MPI version.
 * Simplified C API based on stdio.h FILE*
 * @param name input stream name
 * @param mode "r" (Read), "w" (Write), "a" (Append, not yet supported)
 * @param comm MPI communicator establishing domain for adios2_FILE*
 * @return success: adios2_FILE* handler to adios2 stream, failure: NULL
 */
adios2_FILE *adios2_fopen(const char *name, const char *mode, MPI_Comm comm);

/**
 * Open and adios2 stream adios2_FILE*, MPI version. This version allows for
 * runtime setting in a config file for a particular io.
 * Simplified C API based on stdio.h FILE*
 * @param name input stream name
 * @param mode "r" (Read), "w" (Write), "a" (Append, not yet supported)
 * @param comm MPI communicator establishing domain for adios2_FILE*
 * @param config_file adios2 runtime configuration file name (must has .xml
 * extension)
 * @param io_in_config_file specific io name related to this adios2_FILE*
 * stream. Can be different from first argument `name`.
 * @return success: adios2_FILE* handler to adios2 stream, failure: NULL
 */
adios2_FILE *adios2_fopen_config(const char *name, const char *mode,
                                 MPI_Comm comm, const char *config_file,
                                 const char *io_in_config_file);
#else
/**
 * Open and adios2 stream adios2_FILE*, non-MPI version. This version allows for
 * runtime setting in a config file for a particular io.
 * Simplified C API based on stdio.h FILE*
 * @param name input stream name
 * @param mode "r" (Read), "w" (Write), "a" (Append, not yet supported)
 * @return success: adios2_FILE* handler to adios2 stream, failure: NULL
 */
adios2_FILE *adios2_fopen(const char *name, const char *mode);

/**
 * Open and adios2 stream adios2_FILE*, non-MPI version. This version allows for
 * runtime setting in a config file for a particular io.
 * Simplified C API based on stdio.h FILE*
 * @param name input stream name
 * @param mode "r" (Read), "w" (Write), "a" (Append, not yet supported)
 * @param config_file adios2 runtime configuration file name (must has .xml
 * extension)
 * @param io_in_config_file specific io name related to this adios2_FILE*
 * stream. Can be different from first argument `name`.
 * @return success: adios2_FILE* handler to adios2 stream, failure: NULL
 */
adios2_FILE *adios2_fopen_config(const char *name, const char *mode,
                                 const char *config_file,
                                 const char *io_in_config_file);
#endif

/**
 * Write a self-describing variable to an adios2_FILE* stream
 * @param stream input handler
 * @param name variable name
 * @param type variable type from adios2_type enum
 * @param data variable data pointer
 * @param ndims variable number of dimensions
 * @param shape variable global dimension
 * @param start variable local offset
 * @param count variable local dimension
 * @param end_step adios2_true: advances to the next step, adios2_false: remains
 * in the same step
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_fwrite(adios2_FILE *stream, const char *name,
                           const adios2_type type, const void *data,
                           const size_t ndims, const size_t *shape,
                           const size_t *start, const size_t *count,
                           const adios2_bool end_step);

/**
 * Gets step from stream
 * Based on stdio.h fgets, enables reading on a step-by-step basis in a while
 * or for loop. Read mode only
 * @param step output object current step, adios2_step in an alias to
 * adios2_FILE with scope narrowed to one step
 * @param stream input stream containing steps
 * @return success: adios2_step* handler, failure: NULL
 */
adios2_step *adios2_fgets(adios2_step *step, adios2_FILE *stream);

/**
 * Reads into a pre-allocated pointer a selection piece in dimension. When used
 * with fgets reads current step
 * @param stream input handler
 * @param name variable name
 * @param type variable type from adios2_type enum
 * @param data variable data pointer, must be pre-allocated
 * @param ndims variable number of dimensions
 * @param start variable local offset selection
 * @param count variable local dimension selection from start
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_fread(adios2_FILE *stream, const char *name,
                          const adios2_type type, void *data,
                          const size_t ndims, const size_t *start,
                          const size_t *count);

/**
 * Read accessing steps in random access mode. Not be used with adios2_fgets as
 * it returns an error when reading in stepping mode.
 * @param stream input handler
 * @param name variable name
 * @param type variable type from adios2_type enum
 * @param data variable data pointer, must be pre-allocated
 * @param ndims variable number of dimensions
 * @param start variable local offset selection
 * @param count variable local dimension selection from start
 * @param step_start variable initial step (relative to the variable first
 * appearance, not absolute step in stream)
 * @param step_count variable number of steps form step_start, don't have to be
 * contiguous, necessarily
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_fread_steps(adios2_FILE *stream, const char *name,
                                const adios2_type type, void *data,
                                const size_t ndims, const size_t *start,
                                const size_t *count, const size_t step_start,
                                const size_t step_count);

/**
 * close current adios2_FILE* stream becoming inaccessible
 * @param stream to be close
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_fclose(adios2_FILE *stream);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_C_ADIOS2_C_FILE_H_ */
