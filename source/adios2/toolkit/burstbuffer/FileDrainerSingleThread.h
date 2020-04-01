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
    /* This will create a thread to continuously run and idle if there
       are no operations given.
       Finish() will complete all work then join the thread */
    FileDrainerSingleThread();

    ~FileDrainerSingleThread();

    /** Tell thread to terminate when all draining has finished. */
    void Finish();

    /** Join the thread. Main thread will block until thread terminates */
    void Join();

    static const size_t bufferSize = 4194304; // 4MB

private:
    std::thread th; // created by constructor
    bool finish = false;
    std::mutex finishMutex;
    void DrainThread(); // the thread function
};

} // end namespace burstbuffer
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_BURSTBUFFER_FILEDRAINERSINGLETHREAD_H_ */
