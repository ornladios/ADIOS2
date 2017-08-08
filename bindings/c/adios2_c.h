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

#include <mpi.h>    //TODO: resolve mpi or mpidummy
#include <stddef.h> //size_t

#include "adios2_c_enums.h"

typedef void adios2_ADIOS;
typedef void adios2_IO;
typedef void adios2_Variable;
typedef struct adios2_Engine adios2_Engine;

#ifdef __cplusplus
extern "C" {
#endif

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
 * Create an IO struct pointer handler from ADIOS* handler
 * @param adios ADIOS* handler that owns the IO* handler
 * @param io_name unique name for the newly declared io handler
 * @return valid IO* handler
 */
adios2_IO *adios2_declare_io(adios2_ADIOS *adios, const char *io_name);

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
void adios2_set_param(adios2_IO *io, const char *key, const char *value);

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
void adios2_set_transport_param(adios2_IO *io,
                                const unsigned int transport_index,
                                const char *key, const char *value);

adios2_Engine *adios2_open(adios2_IO *io, const char *name,
                           const adios2_open_mode open_mode, MPI_Comm mpi_comm);

void adios2_write(adios2_Engine *engine, adios2_Variable *variable,
                  const void *values);

void adios2_write_by_name(adios2_Engine *engine, const char *variable_name,
                          const void *values);

void adios2_advance(adios2_Engine *engine);

/**
 * Close all transports in adios2_Engine
 * @param engine handler containing all transports to
 * be closed. engine Becomes NULL after this function is called.
 */
void adios2_close(adios2_Engine *engine);

void adios2_close_by_index(adios2_Engine *engine,
                           const unsigned int transport_index);

/**
 * Deallocate adios pointer
 * @param adios input to be deallocated, all destrcutors called
 */
void adios2_finalize(adios2_ADIOS *adios);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* ADIOS2_BINDINGS_C_ADIOS2_C_H_ */
