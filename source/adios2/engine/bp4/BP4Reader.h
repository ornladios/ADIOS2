/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Reader.h
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_ENGINE_BP4_BP4READER_H_
#define ADIOS2_ENGINE_BP4_BP4READER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/bp4/BP4.h" //format::BP4Deserializer
#include "adios2/toolkit/transportman/TransportMan.h"

namespace adios2
{
namespace core
{
namespace engine
{

class BP4Reader : public Engine
{

public:
    /**
     * Unique constructor
     * @param io
     * @param name
     * @param openMode only read
     * @param mpiComm
     */
    BP4Reader(IO &io, const std::string &name, const Mode mode,
              MPI_Comm mpiComm);

    virtual ~BP4Reader() = default;

    StepStatus BeginStep(StepMode mode = StepMode::NextAvailable,
                         const float timeoutSeconds = -1.0) final;

    size_t CurrentStep() const final;

    void EndStep() final;

    void PerformGets() final;

private:
    format::BP4Deserializer m_BP4Deserializer;
    transportman::TransportMan m_FileManager;
    transportman::TransportMan m_SubFileManager;
    /* transport manager for managing the metadata index file */
    transportman::TransportMan m_FileMetadataIndexManager;

    /** used for per-step reads, TODO: to be moved to BP4Deserializer */
    size_t m_CurrentStep = 0;
    bool m_FirstStep = true;

    void Init();
    void InitTransports();
    void InitBuffer();

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1) final;

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);

    template <class T>
    void ReadVariableBlocks(Variable<T> &variable);

#define declare_type(T)                                                        \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    DoAllStepsBlocksInfo(const Variable<T> &variable) const final;             \
                                                                               \
    std::vector<typename Variable<T>::Info> DoBlocksInfo(                      \
        const Variable<T> &variable, const size_t step) const final;

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP4_BP4READER_H_ */
