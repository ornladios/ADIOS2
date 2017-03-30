/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDescriptor.h uses POSIX as the underlying library
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef FILEDESCRIPTOR_H_
#define FILEDESCRIPTOR_H_

#include "core/Transport.h"

namespace adios
{
namespace transport
{

/**
 * File descriptor transport using the POSIX library
 */
class FileDescriptor : public Transport
{

public:
  FileDescriptor(MPI_Comm mpiComm, const bool debugMode);

  ~FileDescriptor();

  void Open(const std::string name, const std::string accessMode);

  void Write(const char *buffer, std::size_t size);

  void Close();

private:
  int m_FileDescriptor = -1; ///< file descriptor returned by POSIX open
};

} // end namespace transport
} // end namespace
#endif /* FILEDESCRIPTOR_H_ */
