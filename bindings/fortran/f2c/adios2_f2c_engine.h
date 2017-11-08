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
               ADIOS2_BEGIN_STEP_F2C)(adios2_Engine **engine, int *ierr);

void FC_GLOBAL(adios2_put_sync_f2c,
               ADIOS2_PUT_SYNC_F2C)(adios2_Engine **engine,
                                    adios2_Variable **variable,
                                    const void *values, int *ierr);

void FC_GLOBAL(adios2_put_deferred_f2c,
               ADIOS2_PUT_DEFERRED_F2C)(adios2_Engine **engine,
                                        adios2_Variable **variable,
                                        const void *values, int *ierr);

void FC_GLOBAL(adios2_end_step_f2c, ADIOS2_END_STEP_F2C)(adios2_Engine **engine,
                                                         int *ierr);

void FC_GLOBAL(adios2_close_f2c, ADIOS2_CLOSE_F2C)(adios2_Engine **engine,
                                                   int *ierr);

void FC_GLOBAL(adios2_finalize_f2c, ADIOS2_FINALIZE_F2C)(adios2_ADIOS **adios,
                                                         int *ierr);

#ifdef __cplusplus
}
#endif

#endif /* BINDINGS_FORTRAN_F2C_ADIOS2_F2C_ENGINE_H_ */
