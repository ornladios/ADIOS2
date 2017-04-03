/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FP.cpp
 *
 *  Created on: Jan 6, 2017
 *      Author: wfg
 */

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

#include "transport/file/FilePointer.h"

namespace adios
{
namespace transport
{

FilePointer::FilePointer(MPI_Comm mpiComm, const bool debugMode)
    : Transport("FILE*", mpiComm, debugMode)
{
}

FilePointer::~FilePointer()
{
  if (m_File != NULL)
    fclose(m_File);
}

void FilePointer::Open(const std::string name, const std::string accessMode)
{
  m_Name = name;
  m_AccessMode = accessMode;

  if (accessMode == "w" || accessMode == "write")
    m_File = fopen(name.c_str(), "w");

  else if (accessMode == "a" || accessMode == "append")
    m_File = fopen(name.c_str(), "a");

  else if (accessMode == "r" || accessMode == "read")
    m_File = fopen(name.c_str(), "r");

  if (m_DebugMode == true)
  {
    if (m_File == NULL)
      throw std::ios_base::failure("ERROR: couldn't open file " + name +
                                   ", "
                                   "in call to Open from File* transport\n");
  }
}

void FilePointer::SetBuffer(char *buffer, std::size_t size)
{
  int status = setvbuf(m_File, buffer, _IOFBF, size);

  if (m_DebugMode == true)
  {
    if (status == 1)
      throw std::ios_base::failure("ERROR: could not set buffer in rank " +
                                   std::to_string(m_RankMPI) + "\n");
  }
}

void FilePointer::Write(const char *buffer, std::size_t size)
{
  fwrite(buffer, sizeof(char), size, m_File);

  if (m_DebugMode == true)
  {
    if (ferror(m_File))
      throw std::ios_base::failure("ERROR: couldn't write to file " + m_Name +
                                   ", in call to File* write\n");
  }
}

void FilePointer::Flush() { fflush(m_File); }

void FilePointer::Close()
{
  fclose(m_File);

  m_IsOpen = false;
}

} // end namespace transport
} // end namespace
