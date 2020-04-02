/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDrainerSingleThread.cpp
 *
 *  Created on: April 1, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#include "FileDrainerSingleThread.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "../../common/ADIOSTypes.h"

/// \endcond

namespace adios2
{
namespace burstbuffer
{

FileDrainerSingleThread::FileDrainerSingleThread() : FileDrainer() {}

FileDrainerSingleThread::~FileDrainerSingleThread() { Join(); }

void FileDrainerSingleThread::SetBufferSize(size_t bufferSizeBytes)
{
    bufferSize = bufferSizeBytes;
}

void FileDrainerSingleThread::Start()
{
    th = std::thread(&FileDrainerSingleThread::DrainThread, this);
}

void FileDrainerSingleThread::Finish()
{
    finishMutex.lock();
    finish = true;
    finishMutex.unlock();
}

void FileDrainerSingleThread::Join()
{
    if (th.joinable())
    {
        Finish();
        th.join();
    }
}

/*
 * This function is running in a separate thread from all other member function
 * calls.
 */
void FileDrainerSingleThread::DrainThread()
{
    std::vector<char> buffer; // fixed, preallocated buffer to read/write data
    buffer.resize(bufferSize);

    /* Copy a block of data from one file to another at the same offset */
    auto lf_Copy = [&](FileDrainOperation &fdo, int fdr, int fdw,
                       size_t count) {
        Read(fdr, count, buffer.data(), fdo.fromFileName);
        Write(fdw, count, buffer.data(), fdo.toFileName);
    };

    std::chrono::duration<double> d(0.100);

    while (true)
    {
        operationsMutex.lock();
        if (operations.empty())
        {
            operationsMutex.unlock();
            finishMutex.lock();
            bool done = finish;
            finishMutex.unlock();
            if (done)
            {
                break;
            }
            std::this_thread::sleep_for(d);
            continue;
        }

        FileDrainOperation &fdo = operations.front();
        operationsMutex.unlock();

        switch (fdo.op)
        {

        case DrainOperation::Copy:
        case DrainOperation::CopyAppend:
        {
            int fdr = GetFileDescriptor(fdo.fromFileName, Mode::Read);
            Mode wMode =
                (fdo.op == DrainOperation::Copy ? Mode::Write : Mode::Append);
            int fdw = GetFileDescriptor(fdo.toFileName, wMode);

            std::cout << "Drain Copy operation from " << fdo.fromFileName
                      << " -> " << fdo.toFileName << " " << fdo.countBytes
                      << " bytes ";
            if (fdo.op == DrainOperation::Copy)
            {
                std::cout << ", offsets: from " << fdo.fromOffset << " to "
                          << fdo.toOffset;
            }
            std::cout << ", fdr= " << fdr << " fdw = " << fdw << std::endl;

            if (fdr != errorState && fdw != errorState)
            {
                try
                {
                    if (fdo.op == DrainOperation::Copy)
                    {
                        Seek(fdr, fdo.fromOffset, fdo.fromFileName);
                        Seek(fdw, fdo.toOffset, fdo.toFileName);
                    }
                    const size_t batches = fdo.countBytes / bufferSize;
                    const size_t remainder = fdo.countBytes % bufferSize;
                    for (size_t b = 0; b < batches; ++b)
                    {
                        lf_Copy(fdo, fdr, fdw, bufferSize);
                    }
                    if (remainder)
                    {
                        lf_Copy(fdo, fdr, fdw, remainder);
                    }
                }
                catch (std::ios_base::failure &e)
                {
                    std::cerr << "ADIOS THREAD ERROR: " << e.what()
                              << std::endl;
                }
            }
            break;
        }
        case DrainOperation::SeekFrom:
        {
            std::cout << "Drain SeekFrom in file " << fdo.fromFileName << " to "
                      << fdo.fromOffset << std::endl;
            int fdr = GetFileDescriptor(fdo.fromFileName, Mode::Read);
            Seek(fdr, fdo.fromOffset, fdo.fromFileName);
            break;
        }
        case DrainOperation::SeekTo:
        {
            std::cout << "Drain SeekTo in file " << fdo.toFileName << " to "
                      << fdo.toOffset << std::endl;
            int fdw = GetFileDescriptor(fdo.toFileName, Mode::Write);
            Seek(fdw, fdo.toOffset, fdo.toFileName);
            break;
        }
        case DrainOperation::SeekEnd:
        {
            std::cout << "Drain Seek to End of file " << fdo.toFileName
                      << std::endl;
            int fdw = GetFileDescriptor(fdo.toFileName, Mode::Write);
            Seek(fdw, 0, fdo.toFileName, SEEK_END);
            break;
        }
        case DrainOperation::WriteAt:
        {
            std::cout << "Drain Write to file " << fdo.toFileName << " "
                      << fdo.countBytes
                      << "bytes of data from memory to offset " << fdo.toOffset
                      << std::endl;
            int fdw = GetFileDescriptor(fdo.toFileName, Mode::Write);
            Seek(fdw, fdo.toOffset, fdo.toFileName);
            Write(fdw, fdo.countBytes, fdo.dataToWrite.data(), fdo.toFileName);
            break;
        }
        case DrainOperation::Write:
        {
            std::cout << "Drain Write to file " << fdo.toFileName << " "
                      << fdo.countBytes << "bytes of data from memory (no seek)"
                      << std::endl;
            int fdw = GetFileDescriptor(fdo.toFileName, Mode::Write);
            Write(fdw, fdo.countBytes, fdo.dataToWrite.data(), fdo.toFileName);
            break;
        }
        default:
            break;
        }
        operationsMutex.lock();
        operations.pop();
        operationsMutex.unlock();
    }

    CloseAll();
}

} // end namespace burstbuffer
} // end namespace adios2
