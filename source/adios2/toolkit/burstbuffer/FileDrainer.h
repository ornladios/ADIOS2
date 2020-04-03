/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDrainer.h
 *
 *  Created on: April 1, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#ifndef ADIOS2_TOOLKIT_BURSTBUFFER_FILEDRAINER_H_
#define ADIOS2_TOOLKIT_BURSTBUFFER_FILEDRAINER_H_

#include <map>
#include <mutex>
#include <queue>
#include <string>

#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace burstbuffer
{

enum class DrainOperation
{
    SeekEnd, // Seek to the end of target file toFileName (for future
             // copyAppend). Seeking to End of fromFile is not allowed
             // since another thread is writing to it
    CopyAt,  // DO NOT USE: Copy countBytes from fromOffset to toOffset (does
             // seek)
    Copy,    // Copy countBytes (without seek)
    WriteAt, // Write data from memory to toFileName directly at offset
    Write,   // Write data from memory to toFileName directly (without seek)
    Create,  // Open file for writing (creat) - only toFile
    Open     // Open file for append - only toFile

};

struct FileDrainOperation
{
    DrainOperation op;
    std::string fromFileName;
    std::string toFileName;
    size_t countBytes;
    size_t fromOffset;
    size_t toOffset;
    std::vector<char> dataToWrite; // memory to write with Write operation

    FileDrainOperation(DrainOperation op, const std::string &fromFileName,
                       const std::string &toFileName, size_t countBytes,
                       size_t fromOffset, size_t toOffset, const void *data);
};

class FileDrainer
{
public:
    FileDrainer();

    virtual ~FileDrainer();

    void AddOperation(FileDrainOperation &operation);
    void AddOperation(DrainOperation op, const std::string &fromFileName,
                      const std::string &toFileName, size_t fromOffset,
                      size_t toOffset, size_t countBytes,
                      const void *data = nullptr);

    void AddOperationSeekEnd(const std::string &toFileName);
    void AddOperationCopyAt(const std::string &fromFileName,
                            const std::string &toFileName, size_t fromOffset,
                            size_t toOffset, size_t countBytes);
    void AddOperationCopy(const std::string &fromFileName,
                          const std::string &toFileName, size_t countBytes);
    void AddOperationWriteAt(const std::string &toFileName, size_t toOffset,
                             size_t countBytes, const void *data);
    void AddOperationWrite(const std::string &toFileName, size_t countBytes,
                           const void *data);
    void AddOperationOpen(const std::string &toFileName, Mode mode);

    /** Create thread */
    virtual void Start() = 0;

    /** Tell thread to terminate when all draining has finished. */
    virtual void Finish() = 0;

    /** Join the thread. Main thread will block until thread terminates */
    virtual void Join() = 0;

    /** turn on verbosity. set rank to differentiate between the output of
     * processes */
    void SetVerbose(int verboseLevel, int rank);

protected:
    std::queue<FileDrainOperation> operations;
    std::mutex operationsMutex;
    std::map<std::string, int> fileDescriptorMap;

    /** rank of process just for stdout/stderr messages */
    int m_Rank = 0;
    int m_Verbose = 0;
    static const int errorState = -1;

    int GetFileDescriptor(const std::string &path, const Mode mode);
    void CloseAll();

    int Open(const std::string &path, const Mode mode);
    void Seek(int fd, size_t offset, const std::string &path,
              int whence = SEEK_SET);
    void Read(int fd, size_t count, char *buffer, const std::string &path);
    void Write(int fd, size_t count, const char *buffer,
               const std::string &path);
    void Close(int fd, const std::string &path);
};

} // end namespace burstbuffer
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_BURSTBUFFER_FILEDRAINER_H_ */
