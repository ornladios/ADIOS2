/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MPIFile.h
 *
 *  Created on: Jan 5, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_TRANSPORT_FILE_MPI_FILE_H_
#define ADIOS2_TRANSPORT_FILE_MPI_FILE_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMPICommOnly.h"

namespace adios
{
namespace transport
{

/**
 * Class that defines a transport method using C++ file streams
 */
class MPI_File : public Transport
{

public:
    MPI_File(MPI_Comm mpiComm, const bool debugMode);

    ~MPI_File();

    void Open(const std::string streamName, const std::string accessMode);

    void SetBuffer(char *buffer, std::size_t size);

    void Write(const char *buffer, std::size_t size);

    void Flush();

    void Close();

private:
    MPI_File m_MPIFile; ///< MPI File
};

} // end namespace transport
} // end namespace

#endif /* ADIOS2_TRANSPORT_FILE_MPI_FILE_H_ */
