/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDrainerSingleThread.h
 *
 *  Created on: April 1, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#ifndef ADIOS2_TOOLKIT_BURSTBUFFER_FILEDRAINERSINGLETHREAD_H_
#define ADIOS2_TOOLKIT_BURSTBUFFER_FILEDRAINERSINGLETHREAD_H_

#include "adios2/toolkit/burstbuffer/FileDrainer.h"

#include <thread>

namespace adios2
{
namespace burstbuffer
{

class FileDrainerSingleThread : public FileDrainer
{

public:
    // Size of drain buffer (read/write buffer owned by the thread)
    static constexpr size_t defaultBufferSize = 4194304; // 4MB
    // Sync to disk when draining reaches flush size
    static constexpr size_t defaultFlushSize = 32 * defaultBufferSize; // 128MB

    FileDrainerSingleThread();

    ~FileDrainerSingleThread();

    void SetBufferSize(size_t bytes);
    void SetFlushSize(size_t bytes);

    /** Create thread.
     * This will create a thread to continuously run and idle if there
     *  are no operations given.
     *  Finish() will complete all work then join the thread
     */
    void Start();

    /** Tell thread to terminate when all draining has finished. */
    void Finish();

    /** Join the thread. Main thread will block until thread terminates */
    void Join();

private:
    size_t bufferSize = defaultBufferSize;
    size_t flushSize = defaultFlushSize;
    std::thread th; // created by constructor
    bool finish = false;
    std::mutex finishMutex;
    void DrainThread(); // the thread function
};

} // end namespace burstbuffer
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_BURSTBUFFER_FILEDRAINERSINGLETHREAD_H_ */
