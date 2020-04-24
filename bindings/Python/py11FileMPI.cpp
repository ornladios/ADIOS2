/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11FileMPI.cpp
 */

#include "py11File.h"

#include "adios2/helper/adiosCommMPI.h"

namespace adios2
{
namespace py11
{

File::File(const std::string &name, const std::string mode, MPI_Comm comm,
           const std::string engineType)
: m_Name(name), m_Mode(mode),
  m_Stream(std::make_shared<core::Stream>(
      name, ToMode(mode), helper::CommDupMPI(comm), engineType, "Python"))
{
}

File::File(const std::string &name, const std::string mode, MPI_Comm comm,
           const std::string &configFile, const std::string ioInConfigFile)
: m_Name(name), m_Mode(mode),
  m_Stream(std::make_shared<core::Stream>(name, ToMode(mode),
                                          helper::CommDupMPI(comm), configFile,
                                          ioInConfigFile, "Python"))
{
}

} // end namespace py11
} // end namespace adios2
