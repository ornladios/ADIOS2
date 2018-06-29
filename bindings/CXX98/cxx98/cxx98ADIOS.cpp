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

#include <adios2_c.h>

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

ADIOS::ADIOS(MPI_Comm comm, const bool debugMode) : ADIOS("", comm, debugMode)
{
}

ADIOS::ADIOS(const std::string &configFile, const bool debugMode)
: ADIOS(configFile, MPI_COMM_SELF, debugMode)
{
}

ADIOS::ADIOS(const bool debugMode) : ADIOS("", MPI_COMM_SELF, debugMode) {}

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

ADIOS::~ADIOS()
{
    if (m_ADIOS != NULL)
    {
        adios2_finalize(m_ADIOS);
        m_ADIOS = NULL;
    }
}

ADIOS::operator bool() const { return m_ADIOS == NULL ? false : true; }

IO ADIOS::DeclareIO(const std::string &name)
{
    return IO(*adios2_declare_io(m_ADIOS, name.data()));
}

IO ADIOS::AtIO(const std::string &name)
{
    return IO(*adios2_at_io(m_ADIOS, name.c_str()));
}

void ADIOS::FlushAll() { adios2_flush_all(m_ADIOS); }

// Disabled copy constructor
ADIOS::ADIOS(const ADIOS &adios) : m_ADIOS(NULL) {}

} // end namespace cxx03
} // end namespace adios2
