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
               ADIOS2_BEGIN_STEP_F2C)(adios2_engine **engine,
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
void FC_GLOBAL(adios2_put_f2c, ADIOS2_PUT_F2C)(adios2_engine **engine,
                                               adios2_variable **variable,
                                               const void *data,
                                               const int *launch, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_put(*engine, *variable, data, static_cast<adios2_mode>(*launch));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 put: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_put_by_name_f2c,
               ADIOS2_PUT_BY_NAME_F2C)(adios2_engine **engine, const char *name,
                                       const void *data, const int *launch,
                                       int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_put_by_name(*engine, name, data,
                           static_cast<adios2_mode>(*launch));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 put by name: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_perform_puts_f2c,
               ADIOS2_PERFORM_PUTS_F2C)(adios2_engine **engine, int *ierr)
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
void FC_GLOBAL(adios2_get_f2c, ADIOS2_get_F2C)(adios2_engine **engine,
                                               adios2_variable **variable,
                                               void *data, const int *launch,
                                               int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_get(*engine, *variable, data, static_cast<adios2_mode>(*launch));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 get: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_get_by_name_f2c,
               ADIOS2_get_BY_NAME_F2C)(adios2_engine **engine, const char *name,
                                       void *data, const int *launch, int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_get_by_name(*engine, name, data,
                           static_cast<adios2_mode>(*launch));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 get by name: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_perform_gets_f2c,
               ADIOS2_PERFORM_GETS_F2C)(adios2_engine **engine, int *ierr)
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

void FC_GLOBAL(adios2_end_step_f2c, ADIOS2_END_STEP_F2C)(adios2_engine **engine,
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

void FC_GLOBAL(adios2_flush_f2c, ADIOS2_FLUSH_F2C)(adios2_engine **engine,
                                                   int *ierr)
{
    *ierr = 0;
    try
    {
        adios2_flush(*engine);
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 flush: " << e.what() << "\n";
        *ierr = -1;
    }
}

void FC_GLOBAL(adios2_close_f2c, ADIOS2_CLOSE_F2C)(adios2_engine **engine,
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

void FC_GLOBAL(adios2_current_step_f2c,
               ADIOS2_CURRENT_STEP_F2C)(adios2_engine **engine,
                                        int64_t *current_step, int *ierr)
{
    *ierr = 0;
    try
    {
        *current_step = static_cast<int64_t>(adios2_current_step(*engine));
    }
    catch (std::exception &e)
    {
        std::cerr << "ADIOS2 current step: " << e.what() << "\n";
        *ierr = -1;
    }
}
