/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SstReader.h
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#ifndef ADIOS2_ENGINE_SST_SSTREADER_H_
#define ADIOS2_ENGINE_SST_SSTREADER_H_

#include "adios2/toolkit/sst/sst.h"

#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/format/bp/bp3/BP3Deserializer.h"

namespace adios2
{
namespace core
{
namespace engine
{

class SstReader : public Engine
{

public:
    /**
     * Constructor for sst engine Reader
     * @param adios
     * @param name
     * @param accessMode
     * @param comm
     * @param method
     * @param debugMode
     * @param nthreads
     */
    SstReader(IO &io, const std::string &name, const Mode mode,
              helper::Comm comm);

    virtual ~SstReader();

    StepStatus BeginStep();
    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = -1.0);
    size_t CurrentStep() const final;
    void EndStep();
    void PerformGets();
    void Flush(const int transportIndex = -1) final;

private:
    template <class T>
    void ReadVariableBlocks(Variable<T> &variable);

    template <class T>
    void SstBPPerformGets();
    void Init();
    SstStream m_Input;
    SstMarshalMethod m_WriterMarshalMethod;
    bool m_DefinitionsNotified = false;
    bool m_BetweenStepPairs = false;

    /* --- Used only with BP marshaling --- */
    SstFullMetadata m_CurrentStepMetaData = NULL;
    format::BP3Deserializer *m_BP3Deserializer;
    /* --- Used only with BP marshaling --- */

    struct _SstParams Params;
#define declare_locals(Param, Type, Typedecl, Default)                         \
    Typedecl m_##Param = Default;
    SST_FOREACH_PARAMETER_TYPE_4ARGS(declare_locals)
#undef declare_locals

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;                              \
                                                                               \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    DoAllStepsBlocksInfo(const Variable<T> &variable) const final;             \
                                                                               \
    std::vector<typename Variable<T>::Info> DoBlocksInfo(                      \
        const Variable<T> &variable, const size_t step) const final;

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1) final;

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_SST_SSTREADER_H_ */
