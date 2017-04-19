/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FilePointer.h
 *
 *  Created on: Jan 6, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_TRANSPORT_FILE_FILEPOINTER_H_
#define ADIOS2_TRANSPORT_FILE_FILEPOINTER_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <stdio.h> // FILE*
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Transport.h"

namespace adios
{
namespace transport
{

/**
 * Class that defines a transport method using C file pointer (FP) to streams
 * FILE*
 */
class FilePointer : public Transport
{

public:
    FilePointer(MPI_Comm mpiComm, const bool debugMode);

    ~FilePointer();

    void Open(const std::string &name, const std::string accessMode);

    void SetBuffer(char *buffer, std::size_t size);

    void Write(const char *buffer, std::size_t size);

    void Flush();

    void Close();

private:
    FILE *m_File = NULL; ///< C file pointer
};

} // end namespace transport
} // end namespace

#endif /* ADIOS2_TRANSPORT_FILE_FILEPOINTER_H_ */
