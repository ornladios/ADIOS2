/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileStream.h wrapper of C++ fstream for file I/O
 *
 *  Created on: Oct 18, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_FILE_FILESTREAM_H_
#define ADIOS2_TOOLKIT_TRANSPORT_FILE_FILESTREAM_H_

#include <fstream>

#include "adios2/ADIOSConfig.h"
#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace transport
{

/** File stream transport using C++ fstream */
class FileStream : public Transport
{

public:
    FileStream(MPI_Comm mpiComm, const bool debugMode);

    ~FileStream() = default;

    void Open(const std::string &name, const OpenMode openMode) final;

    void SetBuffer(char *buffer, size_t size) final;

    void Write(const char *buffer, size_t size) final;

    void Flush() final;

    void Close() final;

private:
    /** file stream using fstream library */
    std::fstream m_FileStream;
};

} // end namespace transport
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_TRANSPORT_FILE_FILEPOINTER_H_ */
