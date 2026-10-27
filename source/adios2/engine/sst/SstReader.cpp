/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SstReader.h"
#include "SstParamParser.h"

#include <cstring>
#include <string>

#include "adios2/common/Selection.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosFunctions.h"
#include <adios2-perfstubs-interface.h>

namespace adios2
{
namespace core
{
namespace engine
{

SstReader::SstReader(IO &io, const std::string &name, const Mode mode, helper::Comm comm)
: Engine("SstReader", io, name, mode, std::move(comm))
{
    char *cstr = new char[name.length() + 1];
    std::strcpy(cstr, name.c_str());

    Init();

    m_Input = SstReaderOpen(cstr, &Params, &m_Comm);
    if (!m_Input)
    {
        delete[] cstr;
        helper::Throw<std::runtime_error>("Engine", "SstReader", "SstReader",
                                          "SstReader did not find active "
                                          "Writer contact info in file \"" +
                                              m_Name + SST_POSTFIX +
                                              "\".  Timeout or non-current SST contact file?");
    }

    SstReaderGetParams(m_Input, &m_WriterIsRowMajor);

    delete[] cstr;
    m_IsOpen = true;
}

SstReader::~SstReader()
{
    if (m_IsOpen)
    {
        DestructorClose(m_FailVerbose);
    }

    if (m_BP5Deserializer)
        delete m_BP5Deserializer;
    SstStreamDestroy(m_Input);
}

/**
 * Called if destructor is called on an open engine.  Should warn or take any
 * non-complex measure that might help recover.
 */
void SstReader::DestructorClose(bool Verbose) noexcept
{
    if (Verbose)
    {
        std::cerr << "SST Reader \"" << m_Name << "\" Destroyed without a prior Close()."
                  << std::endl;
        std::cerr << "This may result in \"unexpected close\" or \"failed to "
                     "send\" warning from a connected SST Writer."
                  << std::endl;
    }
    m_IsOpen = false;
}

StepStatus SstReader::BeginStep(StepMode Mode, const float timeout_sec)
{
    PERFSTUBS_SCOPED_TIMER_FUNC();

    SstStatusValue result;
    if (m_BetweenStepPairs)
    {
        helper::Throw<std::logic_error>("Engine", "SstReader", "BeginStep",
                                        "BeginStep() is called a second time "
                                        "without an intervening EndStep()");
    }

    switch (Mode)
    {
    case adios2::StepMode::Append:
    case adios2::StepMode::Update:
        helper::Throw<std::invalid_argument>(
            "Engine", "SstReader", "BeginStep",
            "SstReader::BeginStep inappropriate StepMode specified");
    case adios2::StepMode::Read:
        break;
    }
    // used to inquire for variables in streaming mode
    m_IO.m_ReadStreaming = true;
    result = SstAdvanceStep(m_Input, timeout_sec);
    if (result == SstEndOfStream)
    {
        return StepStatus::EndOfStream;
    }
    if (result == SstTimeout)
    {
        return StepStatus::NotReady;
    }

    if (result != SstSuccess)
    {
        return StepStatus::OtherError;
    }

    m_BetweenStepPairs = true;

    m_CurrentStepMetaData = SstGetCurMetadata(m_Input);
    if (!m_BP5Deserializer)
    {
        m_BP5Deserializer = new format::BP5Deserializer(m_WriterIsRowMajor, Params.IsRowMajor);
        m_BP5Deserializer->m_Engine = this;
    }
    SstMetaMetaList MMList = SstGetNewMetaMetaData(m_Input, SstCurrentStep(m_Input));
    //      m_BP5Deserializer->StepInit(m_IO.m_Parameters,
    //                                "in call to BP5::BeginStep", "bp5");
    int i = 0;
    while (MMList && MMList[i].BlockData)
    {
        format::BP5Base::MetaMetaInfoBlock MM;
        MM.MetaMetaID = MMList[i].ID;
        MM.MetaMetaIDLen = MMList[i].IDSize;
        MM.MetaMetaInfo = MMList[i].BlockData;
        MM.MetaMetaInfoLen = MMList[i].BlockSize;
        m_BP5Deserializer->InstallMetaMetaData(MM);
        i++;
    }
    free(MMList);

    SstBlock AttributeBlockList = SstGetAttributeData(m_Input, SstCurrentStep(m_Input));
    i = 0;
    while (AttributeBlockList && AttributeBlockList[i].BlockData)
    {
        m_BP5Deserializer->InstallAttributeData(AttributeBlockList[i].BlockData,
                                                AttributeBlockList[i].BlockSize);
        i++;
    }

    // IsValidStep() is asked about m_EngineStep, so it has to track the stream
    m_IO.m_EngineStep = SstCurrentStep(m_Input);
    m_BP5Deserializer->SetupForStep(SstCurrentStep(m_Input),
                                    static_cast<size_t>(m_CurrentStepMetaData->WriterCohortSize));

    for (int i = 0; i < m_CurrentStepMetaData->WriterCohortSize; i++)
    {
        struct _SstData *tmp = m_CurrentStepMetaData->WriterMetadata[i];
        m_BP5Deserializer->InstallMetaData(tmp->block, tmp->DataSize, i);
    }

    m_IO.ResetVariablesStepSelection(true, "in call to SST Reader BeginStep");

    return StepStatus::OK;
}

size_t SstReader::CurrentStep() const { return SstCurrentStep(m_Input); }

void SstReader::EndStep()
{
    if (!m_BetweenStepPairs)
    {
        helper::Throw<std::logic_error>("Engine", "SstReader", "EndStep",
                                        "EndStep() is called without a successful BeginStep()");
    }
    m_BetweenStepPairs = false;
    PERFSTUBS_SCOPED_TIMER_FUNC();
    if (m_ReaderSelectionsLocked && !m_DefinitionsNotified)
    {
        SstReaderDefinitionLock(m_Input, SstCurrentStep(m_Input));
        m_DefinitionsNotified = true;
    }
    BP5PerformGets();
    SstReleaseStep(m_Input);
    m_InfoMap.clear();
}

void SstReader::Flush(const int transportIndex) {}

// PRIVATE
void SstReader::Init()
{
    SstParamParser Parser;

    Parser.ParseParams(m_IO, Params, m_UserOptions);
}

#define declare_gets(T)                                                                            \
    void SstReader::DoGetSync(Variable<T> &variable, T *data)                                      \
    {                                                                                              \
        if (m_BetweenStepPairs == false)                                                           \
        {                                                                                          \
            helper::Throw<std::logic_error>("Engine", "SstReader", "DoGetSync",                    \
                                            "When using the SST engine in ADIOS2, "                \
                                            "Get() calls must appear between "                     \
                                            "BeginStep/EndStep pairs");                            \
        }                                                                                          \
                                                                                                   \
        /*  DoGetSync() is going to have terrible performance 'cause */                            \
        /*  it's a bad idea in an SST-like environment.  But do */                                 \
        /*  whatever you do forDoGetDeferred() and then PerformGets() */                           \
        DoGetDeferred(variable, data);                                                             \
        if (!variable.m_SingleValue)                                                               \
        {                                                                                          \
            /* Don't need to do gets if this was a SingleValue (in                                 \
             * metadata) */                                                                        \
            PerformGets();                                                                         \
        }                                                                                          \
    }                                                                                              \
                                                                                                   \
    void SstReader::DoGetDeferred(Variable<T> &variable, T *data)                                  \
    {                                                                                              \
        if (m_BetweenStepPairs == false)                                                           \
        {                                                                                          \
            helper::Throw<std::logic_error>("Engine", "SstReader", "DoGetDeferred",                \
                                            "When using the SST engine in ADIOS2, "                \
                                            "Get() calls must appear between "                     \
                                            "BeginStep/EndStep pairs");                            \
        }                                                                                          \
                                                                                                   \
        auto sel = core::InferSelection(variable);                                                 \
        m_BP5Deserializer->QueueGet(variable, data, sel);                                          \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_gets)
#undef declare_gets

#define declare_gets_selection(T)                                                                  \
    void SstReader::DoGetSync(Variable<T> &variable, T *data, const Selection &selection)          \
    {                                                                                              \
        if (m_BetweenStepPairs == false)                                                           \
        {                                                                                          \
            helper::Throw<std::logic_error>("Engine", "SstReader", "DoGetSync",                    \
                                            "When using the SST engine in ADIOS2, "                \
                                            "Get() calls must appear between "                     \
                                            "BeginStep/EndStep pairs");                            \
        }                                                                                          \
        bool need_sync = m_BP5Deserializer->QueueGet(variable, data, selection);                   \
        if (need_sync)                                                                             \
        {                                                                                          \
            BP5PerformGets();                                                                      \
        }                                                                                          \
    }                                                                                              \
                                                                                                   \
    void SstReader::DoGetDeferred(Variable<T> &variable, T *data, const Selection &selection)      \
    {                                                                                              \
        if (m_BetweenStepPairs == false)                                                           \
        {                                                                                          \
            helper::Throw<std::logic_error>("Engine", "SstReader", "DoGetDeferred",                \
                                            "When using the SST engine in ADIOS2, "                \
                                            "Get() calls must appear between "                     \
                                            "BeginStep/EndStep pairs");                            \
        }                                                                                          \
        m_BP5Deserializer->QueueGet(variable, data, selection);                                    \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_gets_selection)
#undef declare_gets_selection

void SstReader::DoGetStructSync(VariableStruct &variable, void *data)
{
    PERFSTUBS_SCOPED_TIMER("BP5Reader::Get");
    auto sel = core::InferSelection(variable);
    bool need_sync = m_BP5Deserializer->QueueGet(variable, data, sel);
    if (need_sync)
        BP5PerformGets();
}

void SstReader::DoGetStructDeferred(VariableStruct &variable, void *data)
{
    PERFSTUBS_SCOPED_TIMER("SstReader::Get");
    auto sel = core::InferSelection(variable);
    m_BP5Deserializer->QueueGet(variable, data, sel);
}

bool SstReader::VarShape(const VariableBase &Var, const size_t Step, Dims &Shape) const
{
    return m_BP5Deserializer->VarShape(Var, Step, Shape);
}

bool SstReader::VariableMinMax(const VariableBase &Var, const size_t Step, MinMaxStruct &MinMax)
{
    return m_BP5Deserializer->VariableMinMax(Var, Step, MinMax);
}

void SstReader::BP5PerformGets()
{
    size_t maxReadSize;
    auto ReadRequests = m_BP5Deserializer->GenerateReadRequests(true, &maxReadSize);
    std::vector<void *> sstReadHandlers;
    for (const auto &Req : ReadRequests)
    {
        void *dp_info = NULL;
        if (m_CurrentStepMetaData->DP_TimestepInfo)
        {
            dp_info = m_CurrentStepMetaData->DP_TimestepInfo[Req.WriterRank];
        }
        auto ret = SstReadRemoteMemory(m_Input, (int)Req.WriterRank, Req.Timestep, Req.StartOffset,
                                       Req.ReadLength, Req.DestinationAddr, dp_info);
        sstReadHandlers.push_back(ret);
    }
    for (const auto &i : sstReadHandlers)
    {
        if (SstWaitForCompletion(m_Input, i) != SstSuccess)
        {
            helper::Throw<std::runtime_error>("Engine", "SstReader", "BP5PerformGets",
                                              "Writer failed before returning data");
        }
    }

    m_BP5Deserializer->FinalizeGets(ReadRequests);
    m_BP5Deserializer->ClearGetState();
}

void SstReader::PerformGets() { BP5PerformGets(); }

void SstReader::DoClose(const int transportIndex) { SstReaderClose(m_Input); }

MinVarInfo *SstReader::MinBlocksInfo(const VariableBase &Var, const size_t Step) const
{
    return (MinVarInfo *)m_BP5Deserializer->MinBlocksInfo(Var, Step);
}

#define declare_type(T)                                                                            \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>> SstReader::DoAllStepsBlocksInfo(   \
        const Variable<T> &variable) const                                                         \
    {                                                                                              \
        helper::Throw<std::invalid_argument>("Engine", "SstReader", "DoAllStepsBlocksInfo",        \
                                             "SST Engine doesn't implement "                       \
                                             "function DoAllStepsBlocksInfo");                     \
        return std::map<size_t, std::vector<typename Variable<T>::BPInfo>>();                      \
    }                                                                                              \
                                                                                                   \
    std::vector<typename Variable<T>::BPInfo> SstReader::DoBlocksInfo(const Variable<T> &variable, \
                                                                      const size_t step) const     \
    {                                                                                              \
        return std::vector<typename Variable<T>::BPInfo>();                                        \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

} // end namespace engine
} // end namespace core
} // end namespace adios2
