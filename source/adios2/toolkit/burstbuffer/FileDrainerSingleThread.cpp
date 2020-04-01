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

#include <iostream>

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

namespace adios2
{
namespace burstbuffer
{

FileDrainerSingleThread::FileDrainerSingleThread()
{
    th = std::thread(&FileDrainerSingleThread::DrainThread, this);
}

FileDrainerSingleThread::~FileDrainerSingleThread() { Join(); }

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

        int fdr = GetFileDescriptor(fdo.fromFileName, Mode::Read);
        int fdw = GetFileDescriptor(fdo.toFileName, Mode::Append);
        if (fdr != errorState && fdw != errorState)
        {
            try
            {
                Seek(fdr, fdo.fromOffset, fdo.fromFileName);
                Seek(fdr, fdo.fromOffset, fdo.toFileName);
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
                std::cerr << "ADIOS THREAD ERROR: " << e.what() << std::endl;
            }
        }

        operationsMutex.lock();
        operations.pop();
        operationsMutex.unlock();
    }

    CloseAll();
}

} // end namespace burstbuffer
} // end namespace adios2
