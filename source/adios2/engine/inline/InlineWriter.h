/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_ENGINE_INLINEMPIWRITER_H_
#define ADIOS2_ENGINE_INLINEMPIWRITER_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosComm.h"

#include <vector>

namespace adios2
{
namespace core
{
namespace engine
{

// The inline reader needs to know about the writer, and vice versa.
// Break cyclic dependency via a forward declaration:
class InlineReader;

class InlineWriter : public Engine
{

public:
    /**
     * Constructor for Writer
     * @param name unique name given to the engine
     * @param accessMode
     * @param comm
     * @param method
     */
    InlineWriter(IO &adios, const std::string &name, const Mode mode, helper::Comm comm);

    ~InlineWriter();

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = -1.0) final;
    size_t CurrentStep() const final;
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

    bool IsInsideStep() const;

    void DestructorClose(bool Verbose) noexcept final{};

private:
    int m_Verbosity = 0;
    int m_WriterRank;                               // my rank in the writers' comm
    size_t m_CurrentStep = static_cast<size_t>(-1); // steps start from 0
    bool m_InsideStep = false;
    bool m_ResetVariables = false; // used when PerformPuts is being used

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;
    const InlineReader *GetReader() const;

#define declare_type(T)                                                                            \
    void DoPutSync(Variable<T> &, const T *) final;                                                \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

#define declare_type(T)                                                                            \
    void DoPut(Variable<T> &, typename Variable<T>::Span &, const bool, const T &) final;
    ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

#define declare_type(T, L)                                                                         \
    T *DoBufferData_##L(const int bufferIdx, const size_t payloadPosition,                         \
                        const size_t bufferID) noexcept final;
    ADIOS2_FOREACH_PRIMITVE_STDTYPE_2ARGS(declare_type)
#undef declare_type

    /**
     * Closes a single transport or all transports
     * @param transportIndex, if -1 (default) closes all transports,
     * otherwise it closes a transport in m_Transport[transportIndex].
     * transportIndex is bounds-checked.
     */
    void DoClose(const int transportIndex = -1) final;

    /**
     * Common function for primitive PutSync, puts variables in buffer
     * @param variable
     * @param values
     */
    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *data);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *data);

    void ResetVariables();

    struct DeferredBlock
    {
        VariableBase *Variable;
        const void *Data;
        size_t DataSize;
        size_t BlockIndex;
    };
    std::vector<DeferredBlock> m_DeferredPuts;
    std::vector<void *> m_InternalBuffers;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_INLINEMPIWRITER_H_ */
