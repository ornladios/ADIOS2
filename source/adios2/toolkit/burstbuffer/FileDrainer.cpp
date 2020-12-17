/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDrainer.cpp
 *
 *  Created on: April 1, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#include "FileDrainer.h"

#include <chrono>
#include <cstdio>
#include <cstring> // std::memcpy
#include <thread>  // std::this_thread::sleep_for

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

namespace adios2
{
namespace burstbuffer
{

FileDrainOperation::FileDrainOperation(DrainOperation op,
                                       const std::string &fromFileName,
                                       const std::string &toFileName,
                                       size_t countBytes, size_t fromOffset,
                                       size_t toOffset, const void *data)
: op(op), fromFileName(fromFileName), toFileName(toFileName),
  countBytes(countBytes), fromOffset(fromOffset), toOffset(toOffset)
{
    if (data)
    {
        dataToWrite.resize(countBytes);
        std::memcpy(dataToWrite.data(), data, countBytes);
    };
}

void FileDrainer::AddOperation(FileDrainOperation &operation)
{
    std::lock_guard<std::mutex> lockGuard(operationsMutex);
    operations.push(operation);
}

void FileDrainer::AddOperation(DrainOperation op,
                               const std::string &fromFileName,
                               const std::string &toFileName, size_t fromOffset,
                               size_t toOffset, size_t countBytes,
                               const void *data)
{
    FileDrainOperation operation(op, fromFileName, toFileName, countBytes,
                                 fromOffset, toOffset, data);
    std::lock_guard<std::mutex> lockGuard(operationsMutex);
    operations.push(operation);
}

void FileDrainer::AddOperationSeekEnd(const std::string &toFileName)
{
    std::string emptyStr;
    AddOperation(DrainOperation::SeekEnd, emptyStr, toFileName, 0, 0, 0);
}
void FileDrainer::AddOperationCopyAt(const std::string &fromFileName,
                                     const std::string &toFileName,
                                     size_t fromOffset, size_t toOffset,
                                     size_t countBytes)
{
    AddOperation(DrainOperation::CopyAt, fromFileName, toFileName, fromOffset,
                 toOffset, countBytes);
}
void FileDrainer::AddOperationCopy(const std::string &fromFileName,
                                   const std::string &toFileName,
                                   size_t countBytes)
{
    AddOperation(DrainOperation::Copy, fromFileName, toFileName, 0, 0,
                 countBytes);
}

void FileDrainer::AddOperationWriteAt(const std::string &toFileName,
                                      size_t toOffset, size_t countBytes,
                                      const void *data)
{
    std::string emptyStr;
    AddOperation(DrainOperation::WriteAt, emptyStr, toFileName, 0, toOffset,
                 countBytes, data);
}

void FileDrainer::AddOperationWrite(const std::string &toFileName,
                                    size_t countBytes, const void *data)
{
    std::string emptyStr;
    AddOperation(DrainOperation::Write, emptyStr, toFileName, 0, 0, countBytes,
                 data);
}

void FileDrainer::AddOperationOpen(const std::string &toFileName, Mode mode)
{
    std::string emptyStr;
    if (mode == Mode::Write)
    {
        AddOperation(DrainOperation::Create, emptyStr, toFileName, 0, 0, 0);
    }
    else if (mode == Mode::Append)
    {
        AddOperation(DrainOperation::Open, emptyStr, toFileName, 0, 0, 0);
    }
    else
    {
        throw std::runtime_error(
            "ADIOS Coding ERROR: FileDrainer::AddOperationOpen() only supports "
            "Write and Append modes\n");
    }
}

void FileDrainer::AddOperationDelete(const std::string &toFileName)
{
    std::string emptyStr;
    AddOperation(DrainOperation::Delete, emptyStr, toFileName, 0, 0, 0);
}

void FileDrainer::SetVerbose(int verboseLevel, int rank)
{
    m_Verbose = verboseLevel;
    m_Rank = rank;
}

} // end namespace burstbuffer
} // end namespace adios2
