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

#include <stdint.h> // int64_t

#ifdef __cplusplus
extern "C" {
#endif

void FC_GLOBAL(adios2_begin_step_f2c,
               ADIOS2_BEGIN_STEP_F2C)(adios2_engine **engine,
                                      const int *step_mode,
                                      const float *timeout_seconds, int *ierr);

// ************** PUT **********************************************************
void FC_GLOBAL(adios2_put_f2c, ADIOS2_PUT_F2C)(adios2_engine **engine,
                                               adios2_variable **variable,
                                               const void *data,
                                               const int *launch, int *ierr);

void FC_GLOBAL(adios2_put_by_name_f2c,
               ADIOS2_PUT_BY_NAME_F2C)(adios2_engine **engine, const char *name,
                                       const void *data, const int *launch,
                                       int *ierr);
void FC_GLOBAL(adios2_perform_puts_f2c,
               ADIOS2_PERFORM_PUTS_F2C)(adios2_engine **engine, int *ierr);
// ****************************************************************************

// ************** GET *********************************************************
void FC_GLOBAL(adios2_get_f2c, ADIOS2_get_F2C)(adios2_engine **engine,
                                               adios2_variable **variable,
                                               void *data, const int *launch,
                                               int *ierr);

void FC_GLOBAL(adios2_get_by_name_f2c,
               ADIOS2_get_BY_NAME_F2C)(adios2_engine **engine, const char *name,
                                       void *data, const int *launch,
                                       int *ierr);

void FC_GLOBAL(adios2_perform_gets_f2c,
               ADIOS2_PERFORM_GETS_F2C)(adios2_engine **engine, int *ierr);

// ****************************************************************************

void FC_GLOBAL(adios2_end_step_f2c, ADIOS2_END_STEP_F2C)(adios2_engine **engine,
                                                         int *ierr);

void FC_GLOBAL(adios2_flush_f2c, ADIOS2_FLUSH_F2C)(adios2_engine **engine,
                                                   int *ierr);

void FC_GLOBAL(adios2_close_f2c, ADIOS2_CLOSE_F2C)(adios2_engine **engine,
                                                   int *ierr);

void FC_GLOBAL(adios2_current_step_f2c,
               ADIOS2_CURRENT_STEP_F2C)(adios2_engine **engine,
                                        int64_t *current_step, int *ierr);

#ifdef __cplusplus
}
#endif

#endif /* BINDINGS_FORTRAN_F2C_ADIOS2_F2C_ENGINE_H_ */
