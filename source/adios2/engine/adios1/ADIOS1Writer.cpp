/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Writer.cpp
 * Class to write files using old adios 1.x library.
 * It requires adios 1.x installed
 *
 *  Created on: Mar 27, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "ADIOS1Writer.h"

#include "adios2/helper/adiosFunctions.h"

// Enable compatibility with ADIOS 1.10 adios_declare_group signature
#if !ADIOS_VERSION_GE(1, 11, 0)
#define adios_stat_default adios_flag_yes
#endif

namespace adios2
{

ADIOS1Writer::ADIOS1Writer(IO &io, const std::string &name, const Mode openMode,
                           MPI_Comm mpiComm)
: Engine("ADIOS1Writer", io, name, openMode, mpiComm),
  m_ADIOS1(io.m_Name, name, mpiComm, io.m_DebugMode)
{
    m_EndMessage = " in call to ADIOS1Writer " + m_Name + " Open\n";
    Init();
}

void ADIOS1Writer::BeginStep() {}

void ADIOS1Writer::EndStep() { m_ADIOS1.Advance(); }

void ADIOS1Writer::Close(const int transportIndex) { m_ADIOS1.Close(); }

// PRIVATE
void ADIOS1Writer::Init()
{
    InitParameters();
    InitTransports();
    m_ADIOS1.Open(m_OpenMode);
}

#define declare_type(T)                                                        \
    void ADIOS1Writer::DoPutSync(Variable<T> &variable, const T *values)       \
    {                                                                          \
        m_ADIOS1.WriteVariable(variable.m_Name, variable.m_ShapeID,            \
                               variable.m_Count, variable.m_Shape,             \
                               variable.m_Start, values);                      \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void ADIOS1Writer::InitParameters()
{
    m_ADIOS1.InitParameters(m_IO.m_Parameters);
}

void ADIOS1Writer::InitTransports()
{
    m_ADIOS1.InitTransports(m_IO.m_TransportsParameters);
}

} // end namespace adios2
