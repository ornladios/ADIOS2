/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileStdio.h wrapper of C/C++ stdio.h for file I/O
 *
 *  Created on: Jan 6, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_FILE_FILEPOINTER_H_
#define ADIOS2_TOOLKIT_TRANSPORT_FILE_FILEPOINTER_H_

#include <cstdio> // FILE*

#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace helper
{
class Comm;
}
namespace transport
{

/** File transport using C stdio FILE* */
class FileStdio : public Transport
{

public:
    FileStdio(helper::Comm const &comm, const bool debugMode);

    ~FileStdio();

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
    /** C File pointer */
    FILE *m_File = nullptr;

    /**
     * Check for std::ferror and throw an exception if true
     * @param hint exception message
     */
    void CheckFile(const std::string hint) const;
};

} // end namespace transport
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORT_FILE_FILEPOINTER_H_ */
