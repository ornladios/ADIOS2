/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileFStream.h wrapper of C++ fstream for file I/O
 *
 *  Created on: Oct 18, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_FILE_FILESTREAM_H_
#define ADIOS2_TOOLKIT_TRANSPORT_FILE_FILESTREAM_H_

#include <fstream>

#include "adios2/common/ADIOSConfig.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace transport
{

/** File stream transport using C++ fstream */
class FileFStream : public Transport
{

public:
    FileFStream(helper::Comm const &comm, const bool debugMode);

    ~FileFStream() = default;

    void Open(const std::string &name, const Mode openMode) final;

    void SetBuffer(char *buffer, size_t size) final;

    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final;

    void Read(char *buffer, size_t size, size_t start = MaxSizeT) final;

    size_t GetSize() final;

    void Flush() final;

    void Close() final;

    void SeekToEnd() final;

    void SeekToBegin() final;

private:
    /** file stream using fstream library */
    std::fstream m_FileStream;

    /**
     * Check if m_FileStream is false after an operation
     * @param hint exception message
     */
    void CheckFile(const std::string hint) const;
};

} // end namespace transport
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORT_FILE_FILEPOINTER_H_ */
