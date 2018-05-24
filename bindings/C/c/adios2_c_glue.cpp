/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_glue.cpp
 *
 *  Created on: Nov 3, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_c_glue.h"

#include "adios2/ADIOSMPI.h"
#include "adios2/core/ADIOS.h"
#include "adios2/highlevelapi/fstream/Stream.h"

#ifdef _WIN32
#pragma warning(disable : 4297) // Windows noexcept default functions
#endif

adios2_adios *adios2_init_config_glue(const char *config_file,
                                      MPI_Comm mpi_comm,
                                      const adios2_debug_mode debug_mode,
                                      const char *host_language)
{
    const bool debugBool = (debug_mode == adios2_debug_mode_on) ? true : false;
    adios2_adios *adios = reinterpret_cast<adios2_adios *>(
        new adios2::ADIOS(config_file, mpi_comm, debugBool, host_language));

    return adios;
}

adios2_adios *adios2_init_glue(MPI_Comm mpi_comm,
                               const adios2_debug_mode debug_mode,
                               const char *host_language)
{
    return adios2_init_config_glue("", mpi_comm, debug_mode, host_language);
}

adios2_adios *adios2_init_config_nompi_glue(const char *config_file,
                                            const adios2_debug_mode debug_mode,
                                            const char *host_language)
{
    return adios2_init_config_glue(config_file, MPI_COMM_SELF, debug_mode,
                                   host_language);
}

adios2_adios *adios2_init_nompi_glue(const adios2_debug_mode debug_mode,
                                     const char *host_language)
{
    return adios2_init_config_glue("", MPI_COMM_SELF, debug_mode,
                                   host_language);
}

adios2_FILE *adios2_fopen_glue(const char *name, const adios2_mode mode,
                               MPI_Comm comm, const char *host_language)
{
    MPI_Comm commCpp;
    if (comm == 0 || comm == MPI_COMM_NULL)
    {
        commCpp = MPI_COMM_SELF;
    }
    else
    {
        commCpp = comm;
    }

    adios2::Stream *streamCpp = nullptr;

    switch (mode)
    {

    case adios2_mode_write:
        streamCpp = new adios2::Stream(name, adios2::Mode::Write, commCpp,
                                       "BPFile", host_language);
        break;

    case adios2_mode_read:
        streamCpp = new adios2::Stream(name, adios2::Mode::Read, commCpp,
                                       "BPFile", host_language);
        break;

    case adios2_mode_append:
        streamCpp = new adios2::Stream(name, adios2::Mode::Append, commCpp,
                                       "BPFile", host_language);
        break;

    default:
        throw std::invalid_argument("ERROR: mode not valid for adios2_fopen " +
                                    std::string(name) + "\n");
        break;
    }

    return reinterpret_cast<adios2_FILE *>(streamCpp);
}

adios2_FILE *adios2_fopen_config_glue(const char *name, const adios2_mode mode,
                                      MPI_Comm comm, const char *config_file,
                                      const char *io_in_config_file,
                                      const char *host_language)
{
    MPI_Comm commCpp;
    if (comm == 0 || comm == MPI_COMM_NULL)
    {
        commCpp = MPI_COMM_SELF;
    }
    else
    {
        commCpp = comm;
    }

    adios2::Stream *streamCpp = nullptr;

    switch (mode)
    {

    case adios2_mode_write:
        streamCpp =
            new adios2::Stream(name, adios2::Mode::Write, commCpp, config_file,
                               io_in_config_file, host_language);
        break;

    case adios2_mode_read:
        streamCpp =
            new adios2::Stream(name, adios2::Mode::Read, commCpp, config_file,
                               io_in_config_file, host_language);
        break;

    case adios2_mode_append:
        streamCpp =
            new adios2::Stream(name, adios2::Mode::Append, commCpp, config_file,
                               io_in_config_file, host_language);
        break;

    default:
        throw std::invalid_argument("ERROR: invalid mode for adios2_fopen " +
                                    std::string(name) + "\n");
        break;
    }

    return reinterpret_cast<adios2_FILE *>(streamCpp);
}

adios2_FILE *adios2_fopen_nompi_glue(const char *name, const adios2_mode mode,
                                     const char *host_language)
{
    return adios2_fopen_glue(name, mode, MPI_COMM_SELF, host_language);
}

adios2_FILE *adios2_fopen_config_nompi_glue(const char *name,
                                            const adios2_mode mode,
                                            const char *config_file,
                                            const char *io_in_config_file,
                                            const char *host_language)
{
    return adios2_fopen_config_glue(name, mode, MPI_COMM_SELF, config_file,
                                    io_in_config_file, host_language);
}
