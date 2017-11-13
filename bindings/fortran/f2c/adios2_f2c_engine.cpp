/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_f2c_engine.cpp
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_f2c_engine.h"

#include <stdexcept>

void FC_GLOBAL(adios2_begin_step_f2c,
               ADIOS2_BEGIN_STEP_F2C)(adios2_Engine **engine, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_begin_step(*engine);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_put_sync_f2c,
               ADIOS2_PUT_SYNC_F2C)(adios2_Engine **engine,
                                    adios2_Variable **variable,
                                    const void *values, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_put_sync(*engine, *variable, values);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_put_deferred_f2c,
               ADIOS2_PUT_DEFERRED_F2C)(adios2_Engine **engine,
                                        adios2_Variable **variable,
                                        const void *values, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_put_deferred(*engine, *variable, values);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_end_step_f2c, ADIOS2_END_STEP_F2C)(adios2_Engine **engine,
                                                         int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_end_step(*engine);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}

void FC_GLOBAL(adios2_close_f2c, ADIOS2_CLOSE_F2C)(adios2_Engine **engine,
                                                   int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_close(*engine);
    }
    catch (std::exception &e)
    {
        *ierr = 1;
    }
}
