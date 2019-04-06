/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TableWriter.cpp
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */

#include "TableWriter.h"
#include "TableWriter.tcc"

#include "adios2/helper/adiosFunctions.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

TableWriter::TableWriter(IO &io, const std::string &name, const Mode mode,
                               MPI_Comm mpiComm)
: Engine("TableWriter", io, name, mode, mpiComm)
{
    MPI_Comm_rank(mpiComm, &m_MpiRank);
    Init();
}

StepStatus TableWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    ++m_CurrentStep;
    return StepStatus::OK;
}

size_t TableWriter::CurrentStep() const
{
    return m_CurrentStep;
}

void TableWriter::PerformPuts()
{
}

void TableWriter::EndStep()
{
    PerformPuts();
}
void TableWriter::Flush(const int transportIndex)
{
}

// PRIVATE

#define declare_type(T)                                                        \
    void TableWriter::DoPutSync(Variable<T> &variable, const T *data)       \
    {                                                                          \
        PutSyncCommon(variable, data);                                     \
    }                                                                          \
    void TableWriter::DoPutDeferred(Variable<T> &variable, const T *data)   \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void TableWriter::Init()
{
    InitParameters();
    InitTransports();
}

void TableWriter::InitParameters()
{
    for (const auto &pair : m_IO.m_Parameters)
    {
        std::string key(pair.first);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string value(pair.second);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (key == "verbose")
        {
            m_Verbosity = std::stoi(value);
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
}

void TableWriter::InitTransports()
{
}

void TableWriter::DoClose(const int transportIndex)
{
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
