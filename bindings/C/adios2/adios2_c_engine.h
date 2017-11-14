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
 */
void adios2_begin_step(adios2_Engine *engine);

//***************** PUT *****************

/**
 * Put a variable in IO using a adios2_Variable handler
 * @param engine handler for engine executing the write
 * @param variable handler for variable from adios2_define_variable
 * @param values application data to be written for this variable
 */
void adios2_put_sync(adios2_Engine *engine, adios2_Variable *variable,
                     const void *values);

void adios2_put_sync_self(adios2_Engine *engine, adios2_Variable *variable);

void adios2_put_sync_by_name(adios2_Engine *engine, const char *variable_name,
                             const void *values);

/**
 * Write a variable using a adios2_Variable handler in deferred mode
 * @param engine handler for engine executing the write
 * @param variable handler for variable from adios2_define_variable
 * @param values application data to be written for this variable
 */
void adios2_put_deferred(adios2_Engine *engine, adios2_Variable *variable,
                         const void *values);

void adios2_put_deferred_self(adios2_Engine *engine, adios2_Variable *variable);

void adios2_put_deferred_by_name(adios2_Engine *engine,
                                 const char *variable_name, const void *values);

void adios2_perform_puts(adios2_Engine *engine);

//***************** GET *****************
void adios2_get_sync(adios2_Engine *engine, adios2_Variable *variable,
                     void *values);

void adios2_get_sync_self(adios2_Engine *engine, adios2_Variable *variable);

void adios2_get_sync_by_name(adios2_Engine *engine, const char *variable_name,
                             void *values);

void adios2_get_deferred(adios2_Engine *engine, adios2_Variable *variable,
                         void *values);

void adios2_get_deferred_self(adios2_Engine *engine, adios2_Variable *variable);

void adios2_get_deferred_by_name(adios2_Engine *engine,
                                 const char *variable_name, void *values);

void adios2_perform_gets(adios2_Engine *engine);

/**
 * terminates interaction with current step
 * @param engine handler executing IO tasks
 */
void adios2_end_step(adios2_Engine *engine);

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

#ifdef __cplusplus
} // end extern C
#endif

#endif /* BINDINGS_C_ADIOS2_ADIOS2_C_ENGINE_H_ */
