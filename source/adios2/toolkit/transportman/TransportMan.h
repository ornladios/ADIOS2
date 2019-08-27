/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TransportMan.h : manages a vector of transports
 *
 *  Created on: May 23, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_TRANSPORTMANAGER_H_
#define ADIOS2_TOOLKIT_TRANSPORT_TRANSPORTMANAGER_H_

#include <future> //std::async, std::future
#include <memory> //std::shared_ptr
#include <string>
#include <unordered_map>
#include <vector>

#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace helper
{
class Comm;
}
namespace transportman
{

class TransportMan
{

public:
    /**
     * Contains all transports
     * <pre>
     * key : unique id
     * value : object derived from Transport base class
     * </pre>
     */
    std::unordered_map<size_t, std::shared_ptr<Transport>> m_Transports;

    /**
     * Unique base constructor
     * @param comm
     * @param debugMode
     */
    TransportMan(helper::Comm const &comm, const bool debugMode);

    virtual ~TransportMan() = default;

    /**
     * Function that will be called from all ranks in communicator, only rank
     * zero creates directories
     * @param fileNames extract directory if needed to be created
     * @param nodeLocal true: all ranks create a directory
     */
    void MkDirsBarrier(const std::vector<std::string> &fileNames,
                       const bool nodeLocal);

    /**
     * OpenFiles passed from fileNames
     * @param fileNames
     * @param openMode
     * @param parametersVector from IO
     * @param profile
     */
    void OpenFiles(const std::vector<std::string> &fileNames,
                   const Mode openMode,
                   const std::vector<Params> &parametersVector,
                   const bool profile);

    /**
     * Async version of OpenFiles
     * @param fileNames
     * @param openMode
     * @param parametersVector
     * @param profile
     * @return
     */
    std::future<void> OpenFilesAsync(
        const std::vector<std::string> &fileNames, const Mode openMode,
        const std::vector<Params> &parametersVector, const bool profile);

    /**
     * Used for sub-files defined by index
     * @param name
     * @param id
     * @param openMode
     * @param parameters
     * @param profile
     */
    void OpenFileID(const std::string &name, const size_t id, const Mode mode,
                    const Params &parameters, const bool profile);

    /**
     * Gets each transport base name from either baseName at Open or name
     * key in
     * parameters
     * Checks if transport name rules IO AddTransport have unique names for
     * every type (for now)
     * @param baseName from Open
     * @param parameters from IO TransportsParameters (from AddTransport
     * function)
     * @return transport base names
     */
    std::vector<std::string>
    GetFilesBaseNames(const std::string &baseName,
                      const std::vector<Params> &parametersVector) const;

    /**
     * m_Type from m_Transports based on derived classes of Transport
     * @return m_Type for each transport in m_Transports (e.g.
     * {FileDescriptor,
     * FilePointer} )
     */
    std::vector<std::string> GetTransportsTypes() noexcept;

    /** Returns a vector of pointer references (not owning the memory) to
     * m_Transports.m_Profiler */
    std::vector<profiling::IOChrono *> GetTransportsProfilers() noexcept;

    /**
     * Write to file transports
     * @param transportIndex
     * @param buffer
     * @param size
     */
    void WriteFiles(const char *buffer, const size_t size,
                    const int transportIndex = -1);

    /**
     * Write data to a specific location in files
     * @param transportIndex
     * @param buffer
     * @param size
     */
    void WriteFileAt(const char *buffer, const size_t size, const size_t start,
                     const int transportIndex = -1);

    size_t GetFileSize(const size_t transportIndex = 0) const;

    /**
     * Read contents from a single file and assign it to buffer
     * @param buffer
     * @param size
     * @param start
     * @param transportIndex
     */
    void ReadFile(char *buffer, const size_t size, const size_t start = 0,
                  const size_t transportIndex = 0);

    /**
     * Flush file or files depending on transport index. Throws an exception
     * if transport is not a file when transportIndex > -1.
     * @param transportIndex -1: all transports, otherwise index in m_Transports
     */
    void FlushFiles(const int transportIndex = -1);

    /**
     * Close file or files depending on transport index. Throws an exception
     * if transport is not a file when transportIndex > -1.
     * @param transportIndex -1: all transports, otherwise index in m_Transports
     */
    void CloseFiles(const int transportIndex = -1);

    /** Checks if all transports are closed */
    bool AllTransportsClosed() const noexcept;

    void SeekToFileEnd(const int transportIndex = 0);

    void SeekToFileBegin(const int transportIndex = 0);

protected:
    helper::Comm const &m_Comm;
    const bool m_DebugMode = false;

    std::shared_ptr<Transport> OpenFileTransport(const std::string &fileName,
                                                 const Mode openMode,
                                                 const Params &parameters,
                                                 const bool profile);

    void CheckFile(
        std::unordered_map<size_t, std::shared_ptr<Transport>>::const_iterator
            itTransport,
        const std::string hint) const;
};

} // end namespace transport
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORT_TRANSPORTMANAGER_H_ */
