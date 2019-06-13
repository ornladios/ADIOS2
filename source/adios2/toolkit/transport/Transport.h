/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Transport.h defines Transport abstract base class
 *
 *  Created on: Oct 6, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_TRANSPORT_H_
#define ADIOS2_TOOLKIT_TRANSPORT_TRANSPORT_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

namespace adios2
{
/** Abstract Base class for transports */
class Transport
{

public:
    const std::string m_Type;    ///< transport type from derived class
    const std::string m_Library; ///< library implementation (POSIX, Mdtm, etc.)
    std::string m_Name; ///< from Open, unique identifier (e.g. filename)
    Mode m_OpenMode = Mode::Undefined; ///< at Open from ADIOSTypes.h
    bool m_IsOpen = false; ///< true: open for communication, false: unreachable
    MPI_Comm m_MPIComm;    ///< current MPI communicator
    int m_RankMPI = 0;     ///< from MPI_Comm_Rank
    int m_SizeMPI = 1;     ///< from MPI_Comm_Size
    profiling::IOChrono m_Profiler; ///< profiles Open, Write/Read, Close

    struct Status
    {
        size_t Bytes;
        bool Running;
        bool Successful;
        // TODO add more thing...time?
    };

    /**
     * Base constructor that all derived classes pass
     * @param type from derived class
     * @param mpiComm passed to m_MPIComm
     * @param debugMode passed to m_DebugMode
     */
    Transport(const std::string type, const std::string library,
              MPI_Comm mpiComm, const bool debugMode);

    virtual ~Transport() = default;

    void InitProfiler(const Mode openMode, const TimeUnit timeUnit);

    /**
     * Opens transport, required before SetBuffer, Write, Read, Flush, Close
     * @param name
     * @param openMode
     */
    virtual void Open(const std::string &name, const Mode openMode) = 0;

    /**
     * If OS buffered (FILE* or fstream), sets the buffer size
     * @param buffer address for OS buffering
     * @param size of OS buffer
     */
    virtual void SetBuffer(char *buffer, size_t size);

    /**
     * Writes to transport. Note that size is non-const due to the nature of
     * underlying transport libraries
     * @param buffer raw data to be written
     * @param size number of bytes to be written
     * @param start starting position for writing (to allow rewind), if not
     * passed then start at current stream position
     */
    virtual void Write(const char *buffer, size_t size,
                       size_t start = MaxSizeT) = 0;

    virtual void IWrite(const char *buffer, size_t size, Status &status,
                        size_t start = MaxSizeT);

    /**
     * Reads from transport "size" bytes from a certain position. Note that size
     * and position and non-const due to the nature of underlying transport
     * libraries
     * @param buffer raw data pointer to put the read bytes (must be
     * preallocated)
     * @param size number of bytes to be read
     * @param start starting position for read, if not passed then start at
     * current stream position
     */
    virtual void Read(char *buffer, size_t size, size_t start = MaxSizeT) = 0;

    virtual void IRead(char *buffer, size_t size, Status &status,
                       size_t start = MaxSizeT);

    /**
     * Returns the size of current data in transport
     * @return size as size_t
     */
    virtual size_t GetSize();

    /** flushes current contents to physical medium without closing */
    virtual void Flush();

    /** closes current file, after this file becomes unreachable */
    virtual void Close() = 0;

protected:
    /** true: turn on exceptions */
    const bool m_DebugMode = false;

    void MkDir(const std::string &fileName);

    void ProfilerStart(const std::string process) noexcept;

    void ProfilerStop(const std::string process) noexcept;

    void CheckName() const;
};

} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORT_TRANSPORT_H_ */
