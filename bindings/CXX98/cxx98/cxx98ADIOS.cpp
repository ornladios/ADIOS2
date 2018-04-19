/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx03ADIOS.cpp
 *
 *  Created on: Apr 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "cxx98ADIOS.h"

namespace adios2
{
namespace cxx98
{

#ifdef ADIOS2_HAVE_MPI
ADIOS::ADIOS(const std::string &configFile, MPI_Comm comm, const bool debugMode)
: m_ADIOS(
      debugMode
          ? adios2_init_config(configFile.c_str(), comm, adios2_debug_mode_on)
          : adios2_init_config(configFile.c_str(), comm, adios2_debug_mode_off))
{
}

ADIOS::ADIOS(MPI_Comm comm, const bool debugMode)
: m_ADIOS(debugMode ? adios2_init(comm, adios2_debug_mode_on)
                    : adios2_init(comm, adios2_debug_mode_off))
{
}
#else
ADIOS::ADIOS(const std::string &configFile, const bool debugMode)
: m_ADIOS(
      debugMode
          ? adios2_init_config_nompi(configFile.c_str(), adios2_debug_mode_on)
          : adios2_init_config_nompi(configFile.c_str(), adios2_debug_mode_off))
{
}

ADIOS::ADIOS(const bool debugMode)
: m_ADIOS(debugMode ? adios2_init_nompi(adios2_debug_mode_on)
                    : adios2_init_nompi(adios2_debug_mode_off))
{
}
#endif

ADIOS::~ADIOS() { adios2_finalize(m_ADIOS); }

IO ADIOS::DeclareIO(const std::string &name)
{
    return IO(*adios2_declare_io(m_ADIOS, name.data()));
}

IO ADIOS::AtIO(const std::string &name)
{
    return IO(*adios2_at_io(m_ADIOS, name.c_str()));
}

void ADIOS::FlushAll() { adios2_flush_all(m_ADIOS); }

} // end namespace cxx03
} // end namespace adios2
