/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Transport.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef ADIOS2_CORE_TRANSPORT_H_
#define ADIOS2_CORE_TRANSPORT_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOS_MPI.h"
#include "adios2/core/IOChrono.h"

namespace adios
{

class Transport
{

public:
    const std::string m_Type; ///< transport type from derived class
    std::string m_Name;       ///< from Open
    std::string m_AccessMode; ///< from Open
    bool m_IsOpen = false;

    MPI_Comm m_MPIComm = MPI_COMM_SELF;

    int m_RankMPI = 0;              ///< current MPI rank process
    int m_SizeMPI = 1;              ///< current MPI processes size
    profiling::IOChrono m_Profiler; ///< profiles Open, Write/Read, Close

    /**
     * Base constructor that all derived classes pass
     * @param
     * @param mpiComm passed to m_MPIComm
     * @param debugMode passed to m_DebugMode
     */
    Transport(const std::string type, MPI_Comm mpiComm, const bool debugMode);

    virtual ~Transport() = default;

    /**
     * Open Output file accesing a mode
     * @param name name of stream or file
     * @param accessMode r or read, w or write, a or append
     */
    virtual void Open(const std::string &name,
                      const std::string accessMode) = 0;

    /**
     * Set buffer and size for a particular transport
     * @param buffer raw data buffer
     * @param size raw data buffer size
     */
    virtual void SetBuffer(char *buffer, std::size_t size);

    /**
     * Write function for a transport
     * @param buffer pointer to buffer to be written
     * @param size size of buffer to be written
     */
    virtual void Write(const char *buffer, std::size_t size) = 0;

    virtual void
    Flush(); ///< flushes current contents to physical medium without
             /// closing the transport

    virtual void Close(); ///< closes current transport and flushes everything,
                          /// transport becomes unreachable

    /**
     * Inits the profiler
     * @param accessMode
     * @param resolution
     */
    virtual void InitProfiler(const std::string accessMode,
                              const Support::Resolutions resolution);

protected:
    const bool m_DebugMode = false; ///< true: turn on exceptions
};

} // end namespace adios

#endif /* ADIOS2_CORE_TRANSPORT_H_ */
