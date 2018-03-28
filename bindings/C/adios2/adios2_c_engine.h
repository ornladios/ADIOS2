/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_engine.h : Engine public API  C bindings
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_C_ADIOS2_ADIOS2_C_ENGINE_H_
#define BINDINGS_C_ADIOS2_ADIOS2_C_ENGINE_H_

#include "adios2/adios2_c_types.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * starts interaction with current step
 * @param engine handler executing IO tasks
 * @param mode
 * @param timeout_seconds
 */
adios2_step_status adios2_begin_step(adios2_engine *engine,
                                     const adios2_step_mode mode,
                                     const float timeout_seconds);

//***************** PUT *****************

/**
 * Put a variable in IO using a adios2_Variable handler
 * @param engine handler for engine executing the write
 * @param variable handler for variable from adios2_define_variable
 * @param values application data to be written for this variable
 */
void adios2_put_sync(adios2_engine *engine, adios2_variable *variable,
                     const void *values);

void adios2_put_sync_self(adios2_engine *engine, adios2_variable *variable);

void adios2_put_sync_by_name(adios2_engine *engine, const char *variable_name,
                             const void *values);

/**
 * Write a variable using a adios2_Variable handler in deferred mode
 * @param engine handler for engine executing the write
 * @param variable handler for variable from adios2_define_variable
 * @param values application data to be written for this variable
 */
void adios2_put_deferred(adios2_engine *engine, adios2_variable *variable,
                         const void *values);

void adios2_put_deferred_self(adios2_engine *engine, adios2_variable *variable);

void adios2_put_deferred_by_name(adios2_engine *engine,
                                 const char *variable_name, const void *values);

void adios2_perform_puts(adios2_engine *engine);

//***************** GET *****************
void adios2_get_sync(adios2_engine *engine, adios2_variable *variable,
                     void *values);

void adios2_get_sync_self(adios2_engine *engine, adios2_variable *variable);

void adios2_get_sync_by_name(adios2_engine *engine, const char *variable_name,
                             void *values);

void adios2_get_deferred(adios2_engine *engine, adios2_variable *variable,
                         void *values);

void adios2_get_deferred_self(adios2_engine *engine, adios2_variable *variable);

void adios2_get_deferred_by_name(adios2_engine *engine,
                                 const char *variable_name, void *values);

void adios2_perform_gets(adios2_engine *engine);

/**
 * terminates interaction with current step
 * @param engine handler executing IO tasks
 */
void adios2_end_step(adios2_engine *engine);

/**
 * Convenience function to write all current variables
 * defined in IO
 * @param engine handler executing IO tasks
 */
void adios2_write_step(adios2_engine *engine);

/**
 * Explicit engine buffer flush to transports
 * @param engine input
 */
void adios2_flush(adios2_engine *engine);

/**
 * Close all transports in adios2_Engine
 * @param engine handler containing all transports to
 * be closed. engine Becomes NULL after this function is called.
 */
void adios2_close(adios2_engine *engine);

/**
 * Close a particular transport from the index returned by adios2_add_transport
 * @param engine handler containing all transports to
 * be closed. NOTE: engine NEVER becomes NULL due to this function.
 * @param transport_index handler from adios2_add_transport
 */
void adios2_close_by_index(adios2_engine *engine,
                           const unsigned int transport_index);

#ifdef __cplusplus
} // end extern C
#endif

#endif /* BINDINGS_C_ADIOS2_ADIOS2_C_ENGINE_H_ */
