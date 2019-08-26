/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InlineReader.h
 * An inline reader which implements zero-copy passing from writer to reader
 *
 *  Created on: Nov 16, 2018
 *      Author: Aron Helser aron.helser@kitware.com
 */

#ifndef ADIOS2_ENGINE_INLINEREADER_H_
#define ADIOS2_ENGINE_INLINEREADER_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace engine
{

class InlineReader : public Engine
{
public:
    /**
     * Constructor for single BP capsule engine, writes in BP format into a
     * single
     * heap capsule
     * @param name unique name given to the engine
     * @param accessMode
     * @param comm
     * @param method
     * @param debugMode
     * @param hostLanguage
     */
    InlineReader(IO &adios, const std::string &name, const Mode mode,
                 helper::Comm comm);

    ~InlineReader();
    StepStatus BeginStep(StepMode mode = StepMode::Read,
                         const float timeoutSeconds = -1.0) final;
    void PerformGets() final;
    size_t CurrentStep() const final;
    void EndStep() final;

private:
    int m_Verbosity = 0;
    int m_ReaderRank; // my rank in the readers' comm

    // step info should be received from the writer side in BeginStep()
    int m_CurrentStep = -1;

    // EndStep must call PerformGets if necessary
    bool m_NeedPerformGets = false;

    std::string m_WriterID;

    void Init() final; ///< called from constructor, gets the selected Inline
                       /// transport method from settings
    void InitParameters() final;
    void InitTransports() final;

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;                              \
    typename Variable<T>::Info *DoGetBlockSync(Variable<T> &) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1);

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);

    template <class T>
    typename Variable<T>::Info *GetBlockSyncCommon(Variable<T> &variable);

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

#endif /* ADIOS2_ENGINE_INLINEREADER_H_ */
