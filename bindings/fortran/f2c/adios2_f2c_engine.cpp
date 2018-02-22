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

#include <iostream> //std::cerr
#include <stdexcept>

void FC_GLOBAL(adios2_begin_step_f2c,
               ADIOS2_BEGIN_STEP_F2C)(adios2_Engine **engine,
                                      const int *step_mode,
                                      const float *timeout_seconds, int *ierr)
{
    *ierr = 0;
    try
    {
        *ierr = adios2_begin_step(*engine,
                                  static_cast<adios2_step_mode>(*step_mode),
                                  *timeout_seconds);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 begin_step: " << e.what() << "\n";
        *ierr = -1;
    }
}

// ******** PUTS */
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
        std::cerr << "ADIOS2 put_sync: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_put_sync_by_name_f2c,
               ADIOS2_PUT_SYNC_BY_NAME_F2C)(adios2_Engine **engine,
                                            const char *name,
                                            const void *values, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_put_sync_by_name(*engine, name, values);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 put_sync: " << e.what() << "\n";
        *ierr = -1;
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
        std::cerr << "ADIOS2 put_deferred: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_put_deferred_by_name_f2c,
               ADIOS2_PUT_DEFERRED_BY_NAME_F2C)(adios2_Engine **engine,
                                                const char *name,
                                                const void *values, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_put_deferred_by_name(*engine, name, values);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 put_deferred: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_perform_puts_f2c,
               ADIOS2_PERFORM_PUTS_F2C)(adios2_Engine **engine, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_perform_puts(*engine);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 perform_puts: " << e.what() << "\n";
        *ierr = -1;
    }
}

// ******** GETS */
void FC_GLOBAL(adios2_get_sync_f2c,
               ADIOS2_get_SYNC_F2C)(adios2_Engine **engine,
                                    adios2_Variable **variable, void *values,
                                    int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_get_sync(*engine, *variable, values);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 get_sync: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_get_sync_by_name_f2c,
               ADIOS2_get_SYNC_BY_NAME_F2C)(adios2_Engine **engine,
                                            const char *name, void *values,
                                            int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_get_sync_by_name(*engine, name, values);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 get_sync: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_get_deferred_f2c,
               ADIOS2_get_DEFERRED_F2C)(adios2_Engine **engine,
                                        adios2_Variable **variable,
                                        void *values, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_get_deferred(*engine, *variable, values);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 get_deferred: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_get_deferred_by_name_f2c,
               ADIOS2_get_DEFERRED_BY_NAME_F2C)(adios2_Engine **engine,
                                                const char *name, void *values,
                                                int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_get_deferred_by_name(*engine, name, values);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 get_deferred: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_perform_gets_f2c,
               ADIOS2_PERFORM_GETS_F2C)(adios2_Engine **engine, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_perform_gets(*engine);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 perform_gets: " << e.what() << "\n";
        *ierr = -1;
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
        std::cerr << "ADIOS2 end_step: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_write_step_f2c,
               ADIOS2_WRITE_STEP_F2C)(adios2_Engine **engine, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_write_step(*engine);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 write_step: " << e.what() << "\n";
        *ierr = -1;
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
        std::cerr << "ADIOS2 close: " << e.what() << "\n";
        *ierr = -1;
    }
}
