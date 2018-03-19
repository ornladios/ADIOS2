/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Reader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "ADIOS1Reader.h"
#include "ADIOS1Reader.tcc"

#include <adios_error.h>

#include "adios2/helper/adiosFunctions.h" // CSVToVector

namespace adios2
{

ADIOS1Reader::ADIOS1Reader(IO &io, const std::string &name, const Mode mode,
                           MPI_Comm mpiComm)
: Engine("ADIOS1Reader", io, name, mode, mpiComm),
  m_ADIOS1(name, mpiComm, io.m_DebugMode)
{
    m_EndMessage = " in call to IO Open ADIOS1Reader " + m_Name + "\n";

    Init();
    m_ADIOS1.Open(io);
}

ADIOS1Reader::~ADIOS1Reader()
{
    /* m_ADIOS1 deconstructor does close and finalize */
}

StepStatus ADIOS1Reader::BeginStep(const StepMode mode,
                                   const float timeoutSeconds)
{
    return m_ADIOS1.AdvanceStep(m_IO, mode, timeoutSeconds);
}

// PRIVATE

#define declare_type(T)                                                        \
    void ADIOS1Reader::DoGetSync(Variable<T> &variable, T *values)             \
    {                                                                          \
        m_ADIOS1.ScheduleReadCommon(variable.m_Name, variable.m_Start,         \
                                    variable.m_Count, variable.m_StepsStart,   \
                                    variable.m_StepsCount,                     \
                                    variable.m_ReadAsLocalValue,               \
                                    variable.m_ReadAsJoined, (void *)values);  \
        m_ADIOS1.PerformReads();                                               \
    }                                                                          \
    void ADIOS1Reader::DoGetDeferred(Variable<T> &variable, T *values)         \
    {                                                                          \
        m_ADIOS1.ScheduleReadCommon(variable.m_Name, variable.m_Start,         \
                                    variable.m_Count, variable.m_StepsStart,   \
                                    variable.m_StepsCount,                     \
                                    variable.m_ReadAsLocalValue,               \
                                    variable.m_ReadAsJoined, (void *)values);  \
        m_NeedPerformGets = true;                                              \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void ADIOS1Reader::PerformGets()
{
    m_ADIOS1.PerformReads();
    m_NeedPerformGets = false;
}

size_t ADIOS1Reader::CurrentStep() const { return m_ADIOS1.CurrentStep(); }

void ADIOS1Reader::EndStep()
{
    if (m_NeedPerformGets)
    {
        PerformGets();
    }
    m_ADIOS1.ReleaseStep();
}

// PRIVATE
void ADIOS1Reader::Init()
{
    if (m_DebugMode)
    {
        if (m_OpenMode != Mode::Read)
        {
            throw std::invalid_argument(
                "ERROR: ADIOS1Reader only supports OpenMode::r (read) access "
                "mode " +
                m_EndMessage);
        }
    }
    InitParameters();
    InitTransports();
}

void ADIOS1Reader::InitParameters()
{
    m_ADIOS1.InitParameters(m_IO.m_Parameters);
}

void ADIOS1Reader::InitTransports()
{
    m_ADIOS1.InitTransports(m_IO.m_TransportsParameters);
}

void ADIOS1Reader::DoClose(const int transportIndex) { m_ADIOS1.Close(); }

} // end namespace adios2
