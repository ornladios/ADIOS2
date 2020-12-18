/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CamBPWriter.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "CamBPWriter.h"
#include "CamBPWriter.tcc"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"
#include "adios2/toolkit/transport/file/FileFStream.h"

#include <ctime>
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

CamBPWriter::CamBPWriter(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("CamBPWriter", io, name, mode, std::move(comm)),
  bp4(io, name, mode, std::move(comm))
{
    TAU_SCOPED_TIMER("CamBPWriter::Open");
}

StepStatus CamBPWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER("CamBPWriter::BeginStep");
    return bp4.BeginStep(mode, timeoutSeconds);
}

size_t CamBPWriter::CurrentStep() const
{
    return bp4.CurrentStep();
}

void CamBPWriter::PerformPuts()
{
    TAU_SCOPED_TIMER("CamBPWriter::PerformPuts");
    bp4.PerformPuts();
}

void CamBPWriter::EndStep()
{
    TAU_SCOPED_TIMER("CamBPWriter::EndStep");
    bp4.EndStep();
}

void CamBPWriter::Flush(const int transportIndex)
{
    TAU_SCOPED_TIMER("CamBPWriter::Flush");
    bp4.Flush(transportIndex);
}


void CamBPWriter::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER("CamBPWriter::Close");
 
}


size_t CamBPWriter::DebugGetDataBufferSize() const
{
    return bp4.DebugGetDataBufferSize();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
