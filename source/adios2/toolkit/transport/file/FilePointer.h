/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FilePointer.h wrapper of C/C++ stdio.h for file I/O
 *
 *  Created on: Jan 6, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_FILE_FILEPOINTER_H_
#define ADIOS2_TOOLKIT_TRANSPORT_FILE_FILEPOINTER_H_

#include <stdio.h> // FILE*

#include "adios2/ADIOSConfig.h"
#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace transport
{

/**
 * Class that defines a transport method using C file pointer (FP) to
 * streams
 * FILE*
 */
class FilePointer : public Transport
{

public:
    FilePointer(MPI_Comm mpiComm, const bool debugMode);

    ~FilePointer();

    void Open(const std::string &name, const OpenMode openMode) final;

    void SetBuffer(char *buffer, size_t size) final;

    void Write(const char *buffer, size_t size) final;

    void Flush();

    void Close();

private:
    /** C File pointer */
    FILE *m_File = nullptr; // NULL or nullptr?
};

} // end namespace transport
} // end namespace

#endif /* ADIOS2_TOOLKIT_TRANSPORT_FILE_FILEPOINTER_H_ */
