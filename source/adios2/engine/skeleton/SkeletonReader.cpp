/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SkeletonReader.cpp
 *
 *  Created on: Jan 04, 2018
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "SkeletonReader.h"
#include "SkeletonReader.tcc"

#include <adios_error.h>

#include "adios2/helper/adiosFunctions.h" // CSVToVector

#include <iostream>

namespace adios2
{

SkeletonReader::SkeletonReader(IO &io, const std::string &name, const Mode mode,
                               MPI_Comm mpiComm)
: Engine("SkeletonReader", io, name, mode, mpiComm)
{
    m_EndMessage = " in call to IO Open SkeletonReader " + m_Name + "\n";
    MPI_Comm_rank(mpiComm, &m_ReaderRank);
    Init();
    if (m_Verbosity == 5)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank << " Open(" << m_Name
                  << ") in constructor." << std::endl;
    }
}

SkeletonReader::~SkeletonReader()
{
    /* m_Skeleton deconstructor does close and finalize */
    if (m_Verbosity == 5)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank << " deconstructor on "
                  << m_Name << "\n";
    }
}

StepStatus SkeletonReader::BeginStep(const StepMode mode,
                                     const float timeoutSeconds)
{
    // step info should be received from the writer side in BeginStep()
    // so this forced increase should not be here
    ++m_CurrentStep;

    if (m_Verbosity == 5)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank
                  << "   BeginStep() new step " << m_CurrentStep << "\n";
    }

    // If we reach the end of stream (writer is gone or explicitly tells the
    // reader)
    // we return EndOfStream to the reader application
    if (m_CurrentStep == 2)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank
                  << "   forcefully returns End of Stream at this step\n";

        return StepStatus::EndOfStream;
    }

    // We should block until a new step arrives or reach the timeout

    // m_IO Variables and Attributes should be defined at this point
    // so that the application can inquire them and start getting data

    return StepStatus::OK;
}

void SkeletonReader::PerformGets()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank
                  << "     PerformGets()\n";
    }
    m_NeedPerformGets = false;
}

void SkeletonReader::EndStep()
{
    // EndStep should call PerformGets() if there are unserved GetDeferred()
    // requests
    if (m_NeedPerformGets)
    {
        PerformGets();
    }

    if (m_Verbosity == 5)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank << "   EndStep()\n";
    }
}

void SkeletonReader::Close(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank << " Close(" << m_Name
                  << ")\n";
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void SkeletonReader::DoGetSync(Variable<T> &variable, T *data)             \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void SkeletonReader::DoGetDeferred(Variable<T> &variable, T *data)         \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    void SkeletonReader::DoGetDeferred(Variable<T> &variable, T &data)         \
    {                                                                          \
        GetDeferredCommon(variable, &data);                                    \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void SkeletonReader::Init()
{
    InitParameters();
    InitTransports();
}

void SkeletonReader::InitParameters()
{
    auto itVerbosity = m_IO.m_Parameters.find("verbose");
    if (itVerbosity != m_IO.m_Parameters.end())
    {
        m_Verbosity = std::stoi(itVerbosity->second);
        if (m_DebugMode)
        {
            if (m_Verbosity < 0 || m_Verbosity > 5)
                throw std::invalid_argument(
                    "ERROR: Method verbose argument must be an "
                    "integer in the range [0,5], in call to "
                    "Open or Engine constructor\n");
        }
    }
}

void SkeletonReader::InitTransports()
{
    // Nothing to process from m_IO.m_TransportsParameters
}

} // end namespace adios2
