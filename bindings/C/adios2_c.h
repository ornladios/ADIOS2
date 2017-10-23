/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c.h : ADIOS2 C bindings declarations
 *
 *  Created on: Mar 13, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_C_ADIOS2_C_H_
#define ADIOS2_BINDINGS_C_ADIOS2_C_H_

#include <stddef.h> //size_t

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/adios2_c_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct adios2_ADIOS adios2_ADIOS;
typedef struct adios2_IO adios2_IO;
typedef struct adios2_Variable adios2_Variable;
typedef struct adios2_Engine adios2_Engine;

/**
 * Create an ADIOS struct pointer handler using a runtime config file in MPI
 * application.
 * @param config_file runtime configuration file, XML format, future: JSON
 * @param mpi_comm MPI communicator from application for ADIOS scope
 * @param debug_mode adios2_debug_mode_on or adios2_debug_mode_off
 * @return valid ADIOS* handler
 */
adios2_ADIOS *adios2_init_config(const char *config_file, MPI_Comm mpi_comm,
                                 const adios2_debug_mode debug_mode);

/**
 * Create an ADIOS struct pointer in MPI application.
 * @param mpi_comm MPI communicator from application for ADIOS scope
 * @param debug_mode adios2_debug_mode_on or adios2_debug_mode_off
 * @return valid ADIOS* handler
 */
adios2_ADIOS *adios2_init(MPI_Comm mpi_comm,
                          const adios2_debug_mode debug_mode);

/**
 * Create an ADIOS struct pointer handler using a runtime config file in serial
 * nonMPI
 * application.
 * @param config_file runtime configuration file, XML format, future: JSON
 * @param debug_mode adios2_debug_mode_on or adios2_debug_mode_off
 * @return valid ADIOS* handler
 */
adios2_ADIOS *adios2_init_config_nompi(const char *config_file,
                                       const adios2_debug_mode debug_mode);

/**
 * Create an ADIOS struct pointer handler in serial nonMPI application.
 * @param debug_mode adios2_debug_mode_on or adios2_debug_mode_off
 * @return valid ADIOS* handler
 */
adios2_ADIOS *adios2_init_nompi(const adios2_debug_mode debug_mode);

/**
 * Function use by language bindings (Fortran) calling the C bindings. DO NOT
 * use this function in applications.
 * @param adios ADIOS* handler
 * @param host_language "Fortran"
 */
void adios2_set_host_language(adios2_ADIOS *adios, const char *host_language);

/**
 * Create an IO struct pointer handler from ADIOS* handler
 * @param adios ADIOS* handler that owns the IO* handler
 * @param io_name unique name for the newly declared io handler
 * @return valid IO* handler
 */
adios2_IO *adios2_declare_io(adios2_ADIOS *adios, const char *io_name);

/**
 * Sets engine type for current io handler
 * @param io handler
 * @param engine_type available engine type
 */
void adios2_set_engine(adios2_IO *io, const char *engine_type);

/**
 * Set a single engine parameter
 * @param io handler
 * @param key parameter key
 * @param value parameter value
 */
void adios2_set_parameter(adios2_IO *io, const char *key, const char *value);

/**
 * Set a transport for the present io
 * @param io handler
 * @param transport_type "File", "WAN"
 * @return transport_index handler used for setting transport parameters or at
 * Close
 */
unsigned int adios2_add_transport(adios2_IO *io, const char *transport_type);

/**
 * Sets a single transport parameter using io and transport_index (from
 * adios2_add_transport) handlers
 * @param io handler
 * @param transport_index handler from adios2_add_transport
 * @param key parameter key
 * @param value parameter value
 */
void adios2_set_transport_parameter(adios2_IO *io,
                                    const unsigned int transport_index,
                                    const char *key, const char *value);

/**
 *
 * @param io handler that owns the variable
 * @param name unique variable name inside IO handler
 * @param type primitive type
 * @param ndims number of dimensions
 * @param shape total MPI dimensions
 * @param start local MPI start (offset)
 * @param count local MPI count
 * @param constant_size adios2_constant_dims_true: shape, start and count are
 * constant, or
 * adios2_constant_size_false
 * @return variable handler
 */
adios2_Variable *
adios2_define_variable(adios2_IO *io, const char *name, const adios2_type type,
                       const size_t ndims, const size_t *shape,
                       const size_t *start, const size_t *count,
                       const adios2_constant_dims constant_dims);

/**
 * Returns a handler to a previously defined variable identified by a unique
 * name
 * @param io handler to variable io owner
 * @param name unique name input
 * @return variable handler if found, else NULL
 */
adios2_Variable *adios2_inquire_variable(adios2_IO *io, const char *name);

/**
 * Create an adios2_Engine, from adios2_IO, that executes all IO operations.
 * Resuse MPI_Comm passed to adios2_ADIOS that created adios2_IO io
 * @param io input that creates the adios2_Engine
 * @param name engine name
 * @param open_mode read, write, append use adios2_open_mode enum
 * @return engine handler
 */
adios2_Engine *adios2_open(adios2_IO *io, const char *name,
                           const adios2_mode open_mode);

/**
 * Create an adios2_Engine, from adios2_IO, that executes all IO operations.
 * Allows passing a new communicator.
 * @param io input that creates the adios2_Engine
 * @param name engine name
 * @param open_mode read, write, append use adios2_open_mode enum
 * @param mpi_comm allows passing a new MPI communicator
 * @return engine handler
 */
adios2_Engine *adios2_open_new_comm(adios2_IO *io, const char *name,
                                    const adios2_mode open_mode,
                                    MPI_Comm mpi_comm);

/**
 * starts interaction with current step
 * @param engine handler executing IO tasks
 */
void adios2_acquire_step(adios2_Engine *engine);

/**
 * Put a variable in IO using a adios2_Variable handler
 * @param engine handler for engine executing the write
 * @param variable handler for variable from adios2_define_variable
 * @param values application data to be written for this variable
 */
void adios2_put_sync(adios2_Engine *engine, adios2_Variable *variable,
                     const void *values);

void adios2_put_sync_self(adios2_Engine *engine, adios2_Variable *variable);

void adios2_put_sync_by_name(adios2_Engine *engine, const char *variableName,
                             const void *values);

/**
 * Write a variable using a adios2_Variable handler
 * @param engine handler for engine executing the write
 * @param variable handler for variable from adios2_define_variable
 * @param values application data to be written for this variable
 */
// void adios2_put_deferred(adios2_Engine *engine, adios2_Variable *variable,
//                         const void *values);
//
// void adios2_put_deferred_self(adios2_Engine *engine, adios2_Variable
// *variable);
//
// void adios2_put_deferred_by_name(adios2_Engine *engine,
//                                 const char *variableName, const void
//                                 *values);

/**
 * terminates interaction with current step
 * @param engine handler executing IO tasks
 */
void adios2_release_step(adios2_Engine *engine);

/**
 * Close all transports in adios2_Engine
 * @param engine handler containing all transports to
 * be closed. engine Becomes NULL after this function is called.
 */
void adios2_close(adios2_Engine *engine);

/**
 * Close a particular transport from the index returned by adios2_add_transport
 * @param engine handler containing all transports to
 * be closed. NOTE: engine NEVER becomes NULL due to this function.
 * @param transport_index handler from adios2_add_transport
 */
void adios2_close_by_index(adios2_Engine *engine,
                           const unsigned int transport_index);

/**
 * Final point for adios2_ADIOS handler.
 * Deallocate adios pointer. Required to avoid memory leaks.
 * @param adios input to be deallocated
 */
void adios2_finalize(adios2_ADIOS *adios);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_ADIOS2_C_H_ */
