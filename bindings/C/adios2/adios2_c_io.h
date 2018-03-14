/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_io.h
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_C_ADIOS2_ADIOS2_C_IO_H_
#define BINDINGS_C_ADIOS2_ADIOS2_C_IO_H_

#include <stddef.h> //size_t

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2_c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Defines a variable inside a corresponding io handler
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
adios2_variable *
adios2_define_variable(adios2_io *io, const char *name, const adios2_type type,
                       const size_t ndims, const size_t *shape,
                       const size_t *start, const size_t *count,
                       const adios2_constant_dims constant_dims, void *data);

/**
 * Returns a handler to a previously defined variable identified by a unique
 * name
 * @param io handler to variable io owner
 * @param name unique name input
 * @return variable handler if found, else NULL
 */
adios2_variable *adios2_inquire_variable(adios2_io *io, const char *name);

/**
 * Remove a variable, DANGEROUS function as it creates dangling pointers
 * @param io handler to variable owner
 * @param name unique variable name input to be removed
 * @return 0: not removed, 1: removed
 */
int adios2_remove_variable(adios2_io *io, const char *name);

/**
 * Remove all variable definitions, DANGEROUS function as it creates dangling
 * pointers
 * @param io handler to variables owner
 */
void adios2_remove_all_variables(adios2_io *io);

/**
 * Defines an attribute inside a corresponding io handler
 * @param io handler that owns the variable
 * @param name unique attribute name inside IO handler
 * @param type primitive type
 * @param data attribute data single value or array
 * @param elements size of data, if 1 it's a single value attribute, else
 * accepts arrays
 * @return attribute handler
 */
adios2_attribute *adios2_define_attribute(adios2_io *io, const char *name,
                                          const adios2_type type,
                                          const void *data,
                                          const size_t elements);

/**
 * Remove an attribute, DANGEROUS function as it creates dangling pointers
 * @param io handler to attribute owner
 * @param name unique attribute name input to be removed
 * @return 0: not removed, 1: removed
 */
int adios2_remove_attribute(adios2_io *io, const char *name);

/**
 * Remove all attribute definitions, DANGEROUS function as it creates dangling
 * pointers
 * @param io handler to attributes owner
 */
void adios2_remove_all_attributes(adios2_io *io);

/**
 * Sets engine type for current io handler
 * @param io handler
 * @param engine_type available engine type
 */
void adios2_set_engine(adios2_io *io, const char *engine_type);

/**
 * Set a single engine parameter
 * @param io handler
 * @param key parameter key
 * @param value parameter value
 */
void adios2_set_parameter(adios2_io *io, const char *key, const char *value);

/**
 * Set a transport for the present io
 * @param io handler
 * @param transport_type "File", "WAN"
 * @return transport_index handler used for setting transport parameters or at
 * Close
 */
unsigned int adios2_add_transport(adios2_io *io, const char *transport_type);

/**
 * Sets a single transport parameter using io and transport_index (from
 * adios2_add_transport) handlers
 * @param io handler
 * @param transport_index handler from adios2_add_transport
 * @param key parameter key
 * @param value parameter value
 */
void adios2_set_transport_parameter(adios2_io *io,
                                    const unsigned int transport_index,
                                    const char *key, const char *value);

/**
 * Create an adios2_Engine, from adios2_IO, that executes all IO operations.
 * Resuse MPI_Comm passed to adios2_ADIOS that created adios2_IO io
 * @param io input that creates the adios2_Engine
 * @param name engine name
 * @param mode read, write, append use adios2_mode enum
 * @return engine handler
 */
adios2_engine *adios2_open(adios2_io *io, const char *name,
                           const adios2_mode mode);

/**
 * Create an adios2_Engine, from adios2_IO, that executes all IO operations.
 * Allows passing a new communicator.
 * @param io input that creates the adios2_Engine
 * @param name engine name
 * @param open_mode read, write, append use adios2_open_mode enum
 * @param mpi_comm allows passing a new MPI communicator
 * @return engine handler
 */
adios2_engine *adios2_open_new_comm(adios2_io *io, const char *name,
                                    const adios2_mode open_mode,
                                    MPI_Comm mpi_comm);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* BINDINGS_C_ADIOS2_ADIOS2_C_IO_H_ */
