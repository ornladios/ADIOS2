/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CamBPWriter.h
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_ENGINE_CamBP_WRITER_H_
#define ADIOS2_ENGINE_CamBP_WRITER_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/burstbuffer/FileDrainerSingleThread.h"
#include "adios2/toolkit/format/bp/bp4/BP4Serializer.h"
#include "adios2/toolkit/transportman/TransportMan.h"

#include "adios2/engine/bp4/BP4Writer.h"

namespace adios2
{
namespace core
{
namespace engine
{

class CamBPWriter : public core::Engine
{

public:
    /**
     * Constructor for file Writer in BP4 format
     * @param name unique name given to the engine
     * @param openMode w (supported), r, a from OpenMode in ADIOSTypes.h
     * @param comm multi-process communicator
     */
    CamBPWriter(IO &io, const std::string &name, const Mode mode,
              helper::Comm comm);

    ~CamBPWriter() = default;

    StepStatus BeginStep(StepMode mode,
                         const float timeoutSeconds = -1.0) final;
    size_t CurrentStep() const final;
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

    size_t DebugGetDataBufferSize() const final;

private:
    BP4Writer bp4;

    void DoClose(const int transportIndex = -1) final;

};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_CamBP_WRITER_H_ */
