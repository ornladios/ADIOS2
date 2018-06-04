/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_c_adios.cpp
 *
 *  Created on: Nov 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2_c_adios.h"

#include "adios2/ADIOSMPI.h"
#include "adios2/core/ADIOS.h"
#include "adios2/helper/adiosFunctions.h"

adios2_adios *adios2_init_config(const char *config_file, MPI_Comm mpi_comm,
                                 const adios2_debug_mode debug_mode)
{
    adios2::helper::CheckForNullptr(
        config_file, "for config_file, in call to adios2_init_config");
    const bool debugBool = (debug_mode == adios2_debug_mode_on) ? true : false;
    adios2_adios *adios = reinterpret_cast<adios2_adios *>(
        new adios2::core::ADIOS(config_file, mpi_comm, debugBool, "C"));

    return adios;
}

adios2_adios *adios2_init(MPI_Comm mpi_comm, const adios2_debug_mode debug_mode)
{
    return adios2_init_config("", mpi_comm, debug_mode);
}
adios2_adios *adios2_init_config_nompi(const char *config_file,
                                       const adios2_debug_mode debug_mode)
{
    return adios2_init_config(config_file, MPI_COMM_SELF, debug_mode);
}

adios2_adios *adios2_init_nompi(const adios2_debug_mode debug_mode)
{
    return adios2_init_config("", MPI_COMM_SELF, debug_mode);
}

adios2_io *adios2_declare_io(adios2_adios *adios, const char *io_name)
{
    adios2::helper::CheckForNullptr(
        adios, "for adios2_adios, in call to adios2_declare_io");
    adios2_io *io = reinterpret_cast<adios2_io *>(
        &reinterpret_cast<adios2::core::ADIOS *>(adios)->DeclareIO(io_name));
    return io;
}

adios2_io *adios2_at_io(adios2_adios *adios, const char *io_name)
{
    adios2::helper::CheckForNullptr(
        adios, "for adios2_adios, in call to adios2_at_io");
    adios2_io *io = reinterpret_cast<adios2_io *>(
        &reinterpret_cast<adios2::core::ADIOS *>(adios)->AtIO(io_name));
    return io;
}

void adios2_flush_all(adios2_adios *adios)
{
    adios2::helper::CheckForNullptr(
        adios, "for adios2_adios, in call to adios2_flush_all");
    reinterpret_cast<adios2::core::ADIOS *>(adios)->FlushAll();
}

void adios2_finalize(adios2_adios *adios)
{
    adios2::helper::CheckForNullptr(
        adios, "for adios2_adios, in call to adios2_finalize");
    delete reinterpret_cast<adios2::core::ADIOS *>(adios);
}
