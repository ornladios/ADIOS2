/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_ENGINE_CAMPAIGNINDEXREADER_H_
#define ADIOS2_ENGINE_CAMPAIGNINDEXREADER_H_

#include "CampaignData.h"
#include "adios2/core/Engine.h"

#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>

namespace adios2
{
namespace core
{
namespace engine
{

/** Reader for hpc-campaign SQLite indexes (.acx).
 * Names are archive/dataset/entry. Shapes and extrema are the summaries in
 * the index; original blocks and per-step statistics are not available.
 * Image/text bytes are read lazily from the corresponding campaign archive
 * or its replicas. ADIOS/HDF5 data reads are not yet supported.
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
    using Entry = CampaignIndexEntry;
    struct Archive : CampaignData
    {
        ~Archive() { Close(); }
    };
    std::unordered_map<std::string, Entry> m_Entries;
    std::unordered_map<std::string, std::unique_ptr<Archive>> m_Archives;
    std::deque<std::function<void()>> m_PendingGets;
    Archive &OpenArchive(const std::string &name);
    void ReadBytes(const Entry &entry, size_t start, size_t count, char *data);
    template <class T>
    void GetSync(Variable<T> &variable, T *data);

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
