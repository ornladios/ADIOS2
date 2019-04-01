/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_adios.h
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_C_C_ADIOS2_C_ADIOS_H_
#define ADIOS2_BINDINGS_C_C_ADIOS2_C_ADIOS_H_

#include "adios2_c_types.h"

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ADIOS2_HAVE_MPI
/**
 * Starting point for MPI apps. Creates an ADIOS handler.
 * MPI collective and it calls MPI_Comm_dup
 * @param comm defines domain scope from application
 * @param debug_mode true: extra user-input debugging information, false:
 * run without checking user-input (stable workflows)
 * @return success: handler, failure: NULL
 */
adios2_adios *adios2_init(MPI_Comm comm, const adios2_debug_mode debug_mode);

/**
 * Starting point for MPI apps. Creates an ADIOS handler allowing a runtime
 * config file.
 * MPI collective and it calls MPI_Comm_dup and MPI_Bcast to pass the
 * configFile contents
 * @param config_file runtime configuration file in xml format
 * @param comm defines domain scope from application
 * @param debug_mode true: extra user-input debugging information, false:
 * run without checking user-input (stable workflows)
 * @return success: handler, failure: NULL
 */
adios2_adios *adios2_init_config(const char *config_file, MPI_Comm comm,
                                 const adios2_debug_mode debug_mode);

#else

/**
 * Initialize an ADIOS struct pointer handler in a serial, non-MPI application.
 * Doesn't require a runtime config file.
 * @param debug_mode adios2_debug_mode_on or adios2_debug_mode_off, adds extra
 * checking to user input to be captured by adios2_error. Use it for stable
 * workflows
 * @return success: handler, failure: NULL
 */
adios2_adios *adios2_init(const adios2_debug_mode debug_mode);

/**
 * Initialize an ADIOS struct pointer handler in a serial, non-MPI application.
 * Doesn't require a runtime config file.
 * @param debug_mode adios2_debug_mode_on or adios2_debug_mode_off, adds extra
 * checking to user input to be captured by adios2_error. Use it for stable
 * workflows
 * @return success: handler, failure: NULL
 */
adios2_adios *adios2_init_config(const char *config_file,
                                 const adios2_debug_mode debug_mode);
#endif

/**
 * Declares a new io handler
 * @param adios owner the io handler
 * @param name unique io identifier within current adios handler
 * @return success: handler, failure: NULL
 */
adios2_io *adios2_declare_io(adios2_adios *adios, const char *name);

/**
 * Retrieves a previously declared io handler by name
 * @param adios owner the io handler
 * @param name unique name for the previously declared io handler
 * @return success: handler, failure: NULL
 */
adios2_io *adios2_at_io(adios2_adios *adios, const char *name);

/**
 * Defines an adios2 supported operator by its type.
 * @param adios owner the op handler
 * @param name unique operator name identifier within current ADIOS object
 * @param type supported ADIOS2 operator type: zfp, sz
 * @return success: handler, failure: NULL
 */
adios2_operator *adios2_define_operator(adios2_adios *adios, const char *name,
                                        const char *type);

/**
 * Retrieves a previously defined operator handler
 * @param adios owner the op handler
 * @param name unique name for the previously defined op handler
 * @return success: handler, failure: NULL
 */
adios2_operator *adios2_inquire_operator(adios2_adios *adios, const char *name);

/**
 * Flushes all adios2_engine in write mode in all adios2_io handlers.
 * If no adios2_io or adios2_engine exists it does nothing.
 * @param adios owner of all io and engines to be flushed
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_flush_all(adios2_adios *adios);

/**
 * Final point for adios handler. Deallocates adios pointer. Required to avoid
 * memory leaks.
 * MPI collective and it calls MPI_Comm_free
 * @param adios handler to be deallocated, must be initialized with
 * adios2_init or adios2_init_config
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_finalize(adios2_adios *adios);

/**
 * DANGER ZONE: removes an io created with adios2_declare_io.
 * Will create dangling pointers for all the handlers inside removed io.
 * NOTE: Use result, not adios2_error to check if the IO was removed.
 * @param result output adios2_true: io not found and not removed, adios2_false:
 * io not found and not removed
 * @param adios owner of io to be removed
 * @param name input unique identifier for io to be removed
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_remove_io(adios2_bool *result, adios2_adios *adios,
                              const char *name);

/**
 * DANGER ZONE: removes all ios created with adios2_declare_io.
 * Will create dangling pointers for all the handlers inside all removed io.
 * @param adios owner of all ios to be removed
 * @return adios2_error 0: success, see enum adios2_error for errors
 */
adios2_error adios2_remove_all_ios(adios2_adios *adios);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_C_ADIOS2_C_ADIOS_H_ */
