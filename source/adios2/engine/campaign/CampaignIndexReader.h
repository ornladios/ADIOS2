/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_ENGINE_CAMPAIGNINDEXREADER_H_
#define ADIOS2_ENGINE_CAMPAIGNINDEXREADER_H_

#include "adios2/core/Engine.h"

namespace adios2
{
namespace core
{
namespace engine
{

/** Metadata-only reader for hpc-campaign SQLite indexes (.acx).
 * Names are archive/dataset/entry. Shapes and extrema are the summaries in
 * the index; original blocks and per-step statistics are not available.
 */
class CampaignIndexReader : public Engine
{
public:
    CampaignIndexReader(IO &io, const std::string &name, Mode mode, helper::Comm comm);
    ~CampaignIndexReader() override = default;
    StepStatus BeginStep(StepMode mode, float timeoutSeconds) final;
    size_t CurrentStep() const final;
    void EndStep() final;
    void PerformGets() final;
    MinVarInfo *MinBlocksInfo(const VariableBase &variable, size_t step) const final;
    bool VarShape(const VariableBase &variable, size_t step, Dims &shape) const final;
    bool VariableMinMax(const VariableBase &variable, size_t step, MinMaxStruct &minmax) final;

private:
    size_t m_Steps = 0;
    size_t m_CurrentStep = 0;
    bool m_InStep = false;
    void Init() final;
    void DoClose(int transportIndex) final;
    size_t DoSteps() const final;
    void DoGetAbsoluteSteps(const VariableBase &variable, std::vector<size_t> &steps) const final;

#define declare_type(T)                                                                            \
    void DoGetSync(Variable<T> &variable, T *data) final;                                          \
    void DoGetDeferred(Variable<T> &variable, T *data) final;                                      \
    std::vector<typename Variable<T>::BPInfo> DoBlocksInfo(const Variable<T> &, size_t)            \
        const final;                                                                               \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>> DoAllStepsBlocksInfo(              \
        const Variable<T> &) const final;                                                          \
    std::vector<std::vector<typename Variable<T>::BPInfo>> DoAllRelativeStepsBlocksInfo(           \
        const Variable<T> &) const final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
};

} // namespace engine
} // namespace core
} // namespace adios2
#endif
