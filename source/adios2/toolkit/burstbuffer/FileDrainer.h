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

struct FileDrainOperation
{
    std::string fromFileName;
    std::string toFileName;
    size_t fromOffset;
    size_t countBytes;
};

class FileDrainer
{
public:
    /** rank of process just for stdout/stderr messages */
    int m_Rank = 0;

    FileDrainer();

    virtual ~FileDrainer();

    void AddOperation(FileDrainOperation &operation);

    /** Tell thread to terminate when all draining has finished. */
    virtual void Finish() = 0;

    /** Join the thread. Main thread will block until thread terminates */
    virtual void Join() = 0;

protected:
    std::queue<FileDrainOperation> operations;
    std::mutex operationsMutex;
    std::map<std::string, int> fileDescriptorMap;

    static const int errorState = -1;

    int GetFileDescriptor(const std::string &path, const Mode mode);
    void CloseAll();

    int Open(const std::string &path, const Mode mode);
    void Seek(int fd, size_t offset, const std::string &path);
    void Read(int fd, size_t count, char *buffer, const std::string &path);
    void Write(int fd, size_t count, const char *buffer,
               const std::string &path);
    void Close(int fd, const std::string &path);
};

} // end namespace burstbuffer
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_BURSTBUFFER_FILEDRAINER_H_ */
