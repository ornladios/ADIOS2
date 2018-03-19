/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_engine.h
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_FORTRAN_F2C_ADIOS2_F2C_ENGINE_H_
#define BINDINGS_FORTRAN_F2C_ADIOS2_F2C_ENGINE_H_

#include "adios2/ADIOSConfig.h"
#include <FC.h> // MACRO FC_GLOBAL, produced by cmake
#include <adios2_c.h>

#ifdef __cplusplus
extern "C" {
#endif

void FC_GLOBAL(adios2_begin_step_f2c,
               ADIOS2_BEGIN_STEP_F2C)(adios2_engine **engine,
                                      const int *step_mode,
                                      const float *timeout_seconds, int *ierr);

// ************** PUT
void FC_GLOBAL(adios2_put_sync_f2c,
               ADIOS2_PUT_SYNC_F2C)(adios2_engine **engine,
                                    adios2_variable **variable,
                                    const void *values, int *ierr);

void FC_GLOBAL(adios2_put_sync_by_name_f2c,
               ADIOS2_PUT_SYNC_BY_NAME_F2C)(adios2_engine **engine,
                                            const char *name,
                                            const void *values, int *ierr);

void FC_GLOBAL(adios2_put_deferred_f2c,
               ADIOS2_PUT_DEFERRED_F2C)(adios2_engine **engine,
                                        adios2_variable **variable,
                                        const void *values, int *ierr);

void FC_GLOBAL(adios2_put_deferred_by_name_f2c,
               ADIOS2_PUT_DEFERRED_BY_NAME_F2C)(adios2_engine **engine,
                                                const char *name,
                                                const void *values, int *ierr);

void FC_GLOBAL(adios2_perform_puts_f2c,
               ADIOS2_PERFORM_PUTS_F2C)(adios2_engine **engine, int *ierr);

// ************** GET
void FC_GLOBAL(adios2_get_sync_f2c,
               ADIOS2_get_SYNC_F2C)(adios2_engine **engine,
                                    adios2_variable **variable, void *values,
                                    int *ierr);

void FC_GLOBAL(adios2_get_sync_by_name_f2c,
               ADIOS2_get_SYNC_BY_NAME_F2C)(adios2_engine **engine,
                                            const char *name, void *values,
                                            int *ierr);

void FC_GLOBAL(adios2_get_deferred_f2c,
               ADIOS2_get_DEFERRED_F2C)(adios2_engine **engine,
                                        adios2_variable **variable,
                                        void *values, int *ierr);

void FC_GLOBAL(adios2_get_deferred_by_name_f2c,
               ADIOS2_get_DEFERRED_BY_NAME_F2C)(adios2_engine **engine,
                                                const char *name, void *values,
                                                int *ierr);

void FC_GLOBAL(adios2_perform_gets_f2c,
               ADIOS2_PERFORM_GETS_F2C)(adios2_engine **engine, int *ierr);

void FC_GLOBAL(adios2_end_step_f2c, ADIOS2_END_STEP_F2C)(adios2_engine **engine,
                                                         int *ierr);

void FC_GLOBAL(adios2_write_step_f2c,
               ADIOS2_WRITE_STEP_F2C)(adios2_engine **engine, int *ierr);

void FC_GLOBAL(adios2_close_f2c, ADIOS2_CLOSE_F2C)(adios2_engine **engine,
                                                   int *ierr);

#ifdef __cplusplus
}
#endif

#endif /* BINDINGS_FORTRAN_F2C_ADIOS2_F2C_ENGINE_H_ */
