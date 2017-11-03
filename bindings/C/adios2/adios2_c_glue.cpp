/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_glue.cpp
 *
 *  Created on: Nov 3, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2/ADIOSMPI.h"
#include "adios2/core/ADIOS.h"

#include "adios2/adios2_c_glue.h"

adios2_ADIOS *adios2_init_config_glue(const char *config_file,
                                      MPI_Comm mpi_comm,
                                      const adios2_debug_mode debug_mode,
                                      const char *host_language)
{
    const bool debugBool = (debug_mode == adios2_debug_mode_on) ? true : false;
    adios2_ADIOS *adios = reinterpret_cast<adios2_ADIOS *>(
        new adios2::ADIOS(config_file, mpi_comm, debugBool, host_language));

    return adios;
}

adios2_ADIOS *adios2_init_glue(MPI_Comm mpi_comm,
                               const adios2_debug_mode debug_mode,
                               const char *host_language)
{
    return adios2_init_config_glue("", mpi_comm, debug_mode, host_language);
}

adios2_ADIOS *adios2_init_config_nompi_glue(const char *config_file,
                                            const adios2_debug_mode debug_mode,
                                            const char *host_language)
{
    return adios2_init_config_glue(config_file, MPI_COMM_SELF, debug_mode,
                                   host_language);
}

adios2_ADIOS *adios2_init_nompi_glue(const adios2_debug_mode debug_mode,
                                     const char *host_language)
{
    return adios2_init_config_glue("", MPI_COMM_SELF, debug_mode,
                                   host_language);
}
