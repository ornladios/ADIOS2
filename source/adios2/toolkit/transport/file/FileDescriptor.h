/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDescriptor.h wrapper of POSIX library functions for file I/O
 *
 *  Created on: Oct 6, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_FILE_FILEDESCRIPTOR_H_
#define ADIOS2_TOOLKIT_TRANSPORT_FILE_FILEDESCRIPTOR_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/toolkit/transport/Transport.h"

namespace adios
{
namespace transport
{

/** File descriptor transport using the POSIX library */
class FileDescriptor : public Transport
{

public:
    FileDescriptor(MPI_Comm mpiComm, const bool debugMode);

    ~FileDescriptor();

    void Open(const std::string &name, const OpenMode openMode) final;

    void Write(const char *buffer, size_t size) final;

    /** Does nothing, each write is supposed to flush */
    void Flush() final;

    void Close() final;

private:
    /** POSIX file handle returned by Open */
    int m_FileDescriptor = -1;
};

} // end namespace transport
} // end namespace
#endif /* ADIOS2_TRANSPORT_FILE_FILEDESCRIPTOR_H_ */
