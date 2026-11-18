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
 * the index; original blocks are retrieved lazily from the source datasets.
 * Image/text bytes are read lazily from the corresponding campaign archive
 * or its replicas. ADIOS/HDF5 engines are opened lazily and cached until Close.
 */
class CampaignIndexReader : public Engine
{
public:
    CampaignIndexReader(IO &io, const std::string &name, Mode mode, helper::Comm comm);
    ~CampaignIndexReader() override;
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
    mutable std::unordered_map<std::string, std::unique_ptr<Archive>> m_Archives;
    struct DatasetEngine
    {
        std::unique_ptr<IO> io;
        Engine *engine;
    };
    mutable std::unordered_map<std::string, DatasetEngine> m_DatasetEngines;
    std::deque<std::function<void()>> m_PendingGets;
    struct BlockStorage
    {
        std::shared_ptr<void> blocks;
        std::vector<const char *> strings;
    };
    mutable std::map<std::pair<std::string, size_t>, BlockStorage> m_BlockStorage;
    template <class T>
    MinVarInfo *FallbackMinBlocksInfo(const Variable<T> &variable, size_t step) const;
    DatasetEngine &OpenDataset(const Entry &entry, size_t dataset) const;
    struct SourceVariable
    {
        Engine *engine;
        VariableBase *variable;
        size_t step;
    };
    SourceVariable FindSource(const VariableBase &variable, size_t step) const;
    template <class T>
    std::vector<typename Variable<T>::BPInfo> BlocksInfo(const Variable<T> &variable,
                                                         size_t step) const;
    std::vector<size_t> ResolveDatasets(Archive &archive, const Entry &entry) const;
    template <class T>
    void ReadDataset(const Entry &entry, Variable<T> &variable, T *data);
    Archive &OpenArchive(const std::string &name) const;
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
