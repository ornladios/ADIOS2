/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TimeSeriesReader.cpp
 *
 *  Created on: Apr 30, 2025
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "TimeSeriesReader.h"
#include "TimeSeriesReader.tcc"

#include "adios2/helper/adiosFunctions.h"
#include <adios2-perfstubs-interface.h>
#include <adios2sys/SystemTools.hxx>
#include <yaml-cpp/yaml.h>

#include <ios>
#include <iostream>
#include <limits>
#include <string>

namespace adios2
{
namespace core
{
namespace engine
{

TimeSeriesReader::TimeSeriesReader(IO &io, const std::string &name, const Mode mode,
                                   helper::Comm comm)
: Engine("TimeSeriesReader", io, name, mode, std::move(comm))
{
    m_ReaderRank = m_Comm.Rank();
    Init();
    m_IsOpen = true;
}

TimeSeriesReader::~TimeSeriesReader()
{
    /* m_TimeSeries deconstructor does close and finalize */
    if (m_IsOpen)
    {
        DestructorClose(m_FailVerbose);
    }
    m_IsOpen = false;
}

StepStatus TimeSeriesReader::BeginStep(const StepMode mode, const float timeoutSeconds)
{
    PERFSTUBS_SCOPED_TIMER("TimeSeriesReader::BeginStep");

    if (m_OpenMode != Mode::Read)
    {
        helper::Throw<std::logic_error>("Engine", "TimeSeriesReader", "BeginStep",
                                        "BeginStep called in random access mode");
    }
    if (m_BetweenStepPairs)
    {
        helper::Throw<std::logic_error>("Engine", "TimeSeriesReader", "BeginStep",
                                        "BeginStep() is called a second time "
                                        "without an intervening EndStep()");
    }

    while (1)
    {
        // either return notready/error from the loop
        // continue when changing from one file to the next
        // break when we succeeded or reached the very end of the stream

        if (m_Engines.empty())
        {
            // move onto new file in time series if available
            CheckForFiles();
            if (!m_TimeSeriesList.entries.empty())
            {
                InitFile(m_TimeSeriesList.entries[0], false);
                m_TimeSeriesList.entries.pop_front();
                ++m_TimeSeriesList.lastUsedEntry;
            }
            else if (m_TimeSeriesList.ended)
                return StepStatus::EndOfStream;
            else
                return StepStatus::NotReady;
        }

        auto e = m_Engines[0];
        StepStatus status = e->BeginStep(mode, timeoutSeconds);
        if (status == StepStatus::NotReady || status == StepStatus::OtherError)
        {
            return StepStatus::NotReady;
        }
        else if (status == StepStatus::EndOfStream)
        {
            // done with this file but we may have more files to process
            m_VarInternalInfo.clear();
            e->Close();
            m_Engines.clear();
            m_IOs.clear();
            // The next file starting step is m_CurrentStep + 1
            m_StartStepInCurrentFile = m_CurrentStep + 1;
            continue;
        }

        // else if (status == StepStatus::OK) ...

        if (m_FirstStep)
        {
            m_FirstStep = false;
        }
        else
        {
            ++m_CurrentStep;
        }
        m_BetweenStepPairs = true;
        m_IO.m_EngineStep = m_CurrentStep;
        m_IO.RemoveAllVariables();
        m_IO.RemoveAllAttributes();
        m_VarInternalInfo.clear();
        ProcessIO(*m_IOs[0], *e);
        break;
    }
    return StepStatus::OK;
}

void TimeSeriesReader::PerformGets()
{
    for (auto &e : m_Engines)
    {
        if (e && *e)
        {
            e->PerformGets();
        }
    }
    m_NeedPerformGets = false;
}

size_t TimeSeriesReader::CurrentStep() const { return m_CurrentStep; }

void TimeSeriesReader::EndStep()
{
    PERFSTUBS_SCOPED_TIMER("TimeSeriesReader::EndStep");
    if (m_OpenMode != Mode::Read)
    {
        helper::Throw<std::logic_error>("Engine", "TimeSeriesReader", "EndStep",
                                        "EndStep called in random access mode");
    }
    if (!m_BetweenStepPairs)
    {
        helper::Throw<std::logic_error>("Engine", "TimeSeriesReader", "EndStep",
                                        "EndStep() is called without a successful BeginStep()");
    }

    // EndStep should call PerformGets() if there are unserved GetDeferred()
    // requests
    if (m_NeedPerformGets)
    {
        PerformGets();
    }
    m_Engines[0]->EndStep();
    m_BetweenStepPairs = false;
}

// PRIVATE

#define declare_type(T)                                                                            \
    void TimeSeriesReader::DoGetSync(Variable<T> &variable, T *data)                               \
    {                                                                                              \
        GetCommon(variable, data, adios2::Mode::Sync);                                             \
    }                                                                                              \
    void TimeSeriesReader::DoGetDeferred(Variable<T> &variable, T *data)                           \
    {                                                                                              \
        GetCommon(variable, data, adios2::Mode::Deferred);                                         \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void TimeSeriesReader::Init()
{
    InitParameters();
    InitTransports();
}

void TimeSeriesReader::InitParameters()
{
    for (const auto &pair : m_IO.m_Parameters)
    {
        std::string key(pair.first);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string value(pair.second);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (key == "verbose")
        {
            m_Verbosity = std::stoi(value);
            if (m_Verbosity < 0 || m_Verbosity > 5)
                helper::Throw<std::invalid_argument>("Engine", "TimeSeriesReader", "InitParameters",
                                                     "Method verbose argument must be an "
                                                     "integer in the range [0,5], in call to "
                                                     "Open or Engine constructor");
        }
    }
}

void TimeSeriesReader::InitTransports()
{
    m_ATSFileDir = adios2sys::SystemTools::GetFilenamePath(m_Name);
    bool ret = CheckForFiles();
    if (m_OpenMode == Mode::ReadRandomAccess)
    {
        if (!ret)
        {
            helper::Throw<std::invalid_argument>("Engine", "TimeSeriesReader", "InitTransports",
                                                 "Error reading TimeSeries file " + m_Name +
                                                     ", in call to Open");
        }
        // open all files now
        for (const auto &tse : m_TimeSeriesList.entries)
        {
            InitFile(tse, true);
            ++m_TimeSeriesList.lastUsedEntry;
        }
        m_TimeSeriesList.entries.clear();
        m_TimeSeriesList.ended = true;
    }
}

bool TimeSeriesReader::CheckForFiles()
{
    if (m_TimeSeriesList.ended)
        return false;

    try
    {
        helper::ParseTimeSeriesFile(m_Comm, m_Name, m_TimeSeriesList);
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "TimeSeries WARNING: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void TimeSeriesReader::ProcessIO(adios2::core::IO &io, adios2::core::Engine &e)
{
    auto vmap = io.GetAvailableVariables();
    auto amap = io.GetAvailableAttributes();

    for (auto &vr : vmap)
    {
        auto vname = vr.first;
        const DataType type = io.InquireVariableType(vname);

        if (type == DataType::Struct)
        {
        }

        else if (type == DataType::String)
        {
            Variable<std::string> *vi = io.InquireVariable<std::string>(vname);
            Variable<std::string> v = DuplicateVariable<std::string>(
                vi, m_IO, m_IOs.size() - 1, m_Engines.size() - 1, vi->m_Min, vi->m_Max);
        }

#define declare_type(T)                                                                            \
    else if (type == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        Variable<T> *vi = io.InquireVariable<T>(vname);                                            \
        MinMaxStruct MinMax;                                                                       \
        T min, max;                                                                                \
        if (TypeHasMinMax(type))                                                                   \
        {                                                                                          \
            if (e.VariableMinMax(*vi, DefaultSizeT, MinMax))                                       \
            {                                                                                      \
                min = *reinterpret_cast<T *>(&MinMax.MinUnion);                                    \
                max = *reinterpret_cast<T *>(&MinMax.MaxUnion);                                    \
            }                                                                                      \
            else                                                                                   \
            {                                                                                      \
                min = vi->m_Min;                                                                   \
                max = vi->m_Max;                                                                   \
            }                                                                                      \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            min = std::numeric_limits<T>::min();                                                   \
            max = std::numeric_limits<T>::max();                                                   \
        }                                                                                          \
        Variable<T> v =                                                                            \
            DuplicateVariable(vi, m_IO, m_IOs.size() - 1, m_Engines.size() - 1, min, max);         \
    }

        ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }

    for (auto &ar : amap)
    {
        auto aname = ar.first;
        const DataType type = io.InquireAttributeType(aname);

        if (type == DataType::Struct)
        {
        }
#define declare_type(T)                                                                            \
    else if (type == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        Attribute<T> *ai = io.InquireAttribute<T>(aname);                                          \
        Attribute<T> v = DuplicateAttribute(ai, m_IO);                                             \
    }

        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
}

void TimeSeriesReader::InitFile(const helper::TimeSeriesEntry &tse, bool process)
{
    static size_t fileCount = 0;
    adios2::core::IO &io = m_IO.m_ADIOS.DeclareIO("TimeSeriesReader" + std::to_string(fileCount));
    if (!tse.remotehost.empty() && !tse.remotepath.empty())
    {
        io.SetParameter("RemoteDataPath", tse.remotepath);
        io.SetParameter("RemoteHost", tse.remotehost);
        io.SetParameter("UUID", tse.uuid);
    }
    adios2::core::Engine &e = io.Open(tse.localpath, m_OpenMode, m_Comm.Duplicate());
    m_StepsCount += e.Steps();
    m_IOs.push_back(&io);
    m_Engines.push_back(&e);
    if (process)
    {
        ProcessIO(io, e);
    }
    ++fileCount;
}

void TimeSeriesReader::DoClose(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "TimeSeries Reader " << m_ReaderRank << " Close(" << m_Name << ")\n";
    }
    for (auto &e : m_Engines)
    {
        if (e)
        {
            e->Close();
        }
    }
    m_IsOpen = false;
}

size_t TimeSeriesReader::FindStep(const VarInternalInfo &vii, const size_t step) const
{
    // std::cout << "TimeSeries Reader " << m_ReaderRank << " FindStep() name = " << vii.name
    //           << "  step = " << step;
    if (m_OpenMode == Mode::ReadRandomAccess)
    {

        size_t fixedstep = (step == adios2::EngineCurrentStep ? 0 : step);
        for (size_t i = 0; i < vii.info.size(); ++i)
        {
            if (vii.info[i].startStep <= fixedstep && vii.info[i].endStep >= fixedstep)
            {
                // std::cout << " -> engine = " << i << std::endl;
                return i;
            }
        }
        // std::cout << " -> engine not found\n";
        return std::numeric_limits<size_t>::max();
    }
    else
    {
        // std::cout << " -> engine = 0 " << std::endl;
        return 0; // there is only one engine with one step at a time
    }
}

MinVarInfo *TimeSeriesReader::MinBlocksInfo(const VariableBase &Var, size_t Step) const
{
    // std::cout << "TimeSeries Reader " << m_ReaderRank << " MinBlocksInfo() name = " << Var.m_Name
    //           << "  step = " << Step << std::endl;
    auto it = m_VarInternalInfo.find(Var.m_Name);
    if (it != m_VarInternalInfo.end())
    {
        auto vii = it->second;
        size_t engineStep = (Step == adios2::EngineCurrentStep ? Var.m_StepsStart : Step);
        size_t idx = FindStep(vii, engineStep);
        engineStep -= vii.info[idx].startStep;
        if (idx < vii.info.size())
        {
            auto viis = vii.info[idx];
            VariableBase *vb = reinterpret_cast<VariableBase *>(viis.originalVar);
            Engine *e = m_Engines[viis.engineIdx];
            // std::cout << "   call engine " << viis.engineIdx
            //           << " MinBlocksInfo() engineStep = " << engineStep << std::endl;
            MinVarInfo *MV = e->MinBlocksInfo(*vb, engineStep);
            if (MV)
            {
                MV->Step = Step;
                return MV;
            }
        }
    }
    return nullptr;
}

bool TimeSeriesReader::VarShape(const VariableBase &Var, const size_t Step, Dims &Shape) const
{
    // std::cout << "TimeSeries Reader " << m_ReaderRank << " VarShape() name = " << Var.m_Name
    //           << "  step = " << Step << std::endl;
    auto it = m_VarInternalInfo.find(Var.m_Name);
    if (it != m_VarInternalInfo.end())
    {
        auto vii = it->second;
        size_t engineStep = (Step == adios2::EngineCurrentStep ? Var.m_StepsStart : Step);
        size_t idx = FindStep(vii, engineStep);
        engineStep -= vii.info[idx].startStep;
        if (idx < vii.info.size())
        {
            auto viis = vii.info[idx];
            VariableBase *vb = reinterpret_cast<VariableBase *>(viis.originalVar);
            Engine *e = m_Engines[viis.engineIdx];
            return e->VarShape(*vb, engineStep, Shape);
        }
    }
    return false;
}

bool TimeSeriesReader::VariableMinMax(const VariableBase &Var, const size_t Step,
                                      MinMaxStruct &MinMax)
{
    // std::cout << "TimeSeries Reader " << m_ReaderRank << " VariableMinMax() name = " <<
    // Var.m_Name
    //           << "  step = " << Step << std::endl;
    auto it = m_VarInternalInfo.find(Var.m_Name);
    if (it != m_VarInternalInfo.end())
    {
        auto vii = it->second;
        size_t engineStep = (Step == adios2::EngineCurrentStep ? Var.m_StepsStart : Step);
        size_t idx = FindStep(vii, engineStep);
        engineStep -= vii.info[idx].startStep;
        if (idx < vii.info.size())
        {
            auto viis = vii.info[idx];
            VariableBase *vb = reinterpret_cast<VariableBase *>(viis.originalVar);
            Engine *e = m_Engines[viis.engineIdx];
            return e->VariableMinMax(*vb, engineStep, MinMax);
        }
    }
    return false;
}

std::string TimeSeriesReader::VariableExprStr(const VariableBase &Var)
{
    // std::cout << "TimeSeries Reader " << m_ReaderRank << " VariableExprStr() name = " <<
    // Var.m_Name
    //           << std::endl;
    auto it = m_VarInternalInfo.find(Var.m_Name);
    if (it != m_VarInternalInfo.end())
    {
        auto vii = it->second;
        auto viis = vii.info[0];
        VariableBase *vb = reinterpret_cast<VariableBase *>(viis.originalVar);
        Engine *e = m_Engines[viis.engineIdx];
        return e->VariableExprStr(*vb);
    }
    return "";
}

size_t TimeSeriesReader::DoSteps() const
{
    // std::cout << "TimeSeries Reader " << m_ReaderRank << " DoSteps() = " << m_StepsCount << "\n";
    return m_StepsCount;
}

#define declare_type(T)                                                                            \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>>                                    \
    TimeSeriesReader::DoAllStepsBlocksInfo(const Variable<T> &variable) const                      \
    {                                                                                              \
        PERFSTUBS_SCOPED_TIMER("TimeSeriesReader::AllStepsBlocksInfo");                            \
        /*std::cout << "TimeSeries Reader " << m_ReaderRank << " DoAllStepsBlocksInfo()\n";*/      \
        std::map<size_t, std::vector<typename Variable<T>::BPInfo>> res;                           \
        auto it = m_VarInternalInfo.find(variable.m_Name);                                         \
        auto vii = it->second;                                                                     \
        for (size_t engineIdx = 0; engineIdx < vii.nEngines; ++engineIdx)                          \
        {                                                                                          \
            auto viis = vii.info[engineIdx];                                                       \
            Variable<T> *v = reinterpret_cast<Variable<T> *>(viis.originalVar);                    \
            Engine *e = m_Engines[viis.engineIdx];                                                 \
            auto m = e->AllStepsBlocksInfo(*v);                                                    \
            /*std::cout << "   engine " << engineIdx << " steps = " << m.size() << "\n"; */        \
            for (auto &pair : m)                                                                   \
            {                                                                                      \
                res[pair.first + vii.info[engineIdx].startStep] = pair.second;                     \
            }                                                                                      \
        }                                                                                          \
        return res;                                                                                \
    }                                                                                              \
                                                                                                   \
    std::vector<std::vector<typename Variable<T>::BPInfo>>                                         \
    TimeSeriesReader::DoAllRelativeStepsBlocksInfo(const Variable<T> &variable) const              \
    {                                                                                              \
        PERFSTUBS_SCOPED_TIMER("TimeSeriesReader::AllRelativeStepsBlocksInfo");                    \
        /* std::cout << "TimeSeries Reader " << m_ReaderRank << "                                  \
         * DoAllRelativeStepsBlocksInfo()\n"; */                                                   \
        std::vector<std::vector<typename Variable<T>::BPInfo>> res;                                \
        auto it = m_VarInternalInfo.find(variable.m_Name);                                         \
        auto vii = it->second;                                                                     \
        for (size_t engineIdx = 0; engineIdx < vii.nEngines; ++engineIdx)                          \
        {                                                                                          \
            auto viis = vii.info[engineIdx];                                                       \
            Variable<T> *v = reinterpret_cast<Variable<T> *>(viis.originalVar);                    \
            Engine *e = m_Engines[viis.engineIdx];                                                 \
            auto vec = e->AllRelativeStepsBlocksInfo(*v);                                          \
            res.insert(res.end(), vec.begin(), vec.end());                                         \
        }                                                                                          \
        return res;                                                                                \
    }                                                                                              \
                                                                                                   \
    std::vector<typename Variable<T>::BPInfo> TimeSeriesReader::DoBlocksInfo(                      \
        const Variable<T> &variable, const size_t step) const                                      \
    {                                                                                              \
        PERFSTUBS_SCOPED_TIMER("TimeSeriesReader::BlocksInfo");                                    \
        /*std::cout << "TimeSeries Reader " << m_ReaderRank << " DoBlocksInfo()\n";     */         \
        std::vector<typename Variable<T>::BPInfo> res;                                             \
        auto it = m_VarInternalInfo.find(variable.m_Name);                                         \
        auto vii = it->second;                                                                     \
        size_t engineStep = (step == adios2::EngineCurrentStep ? variable.m_StepsStart : step);    \
        size_t idx = FindStep(vii, engineStep);                                                    \
        engineStep -= vii.info[idx].startStep;                                                     \
        /* std::cout << "  step = " << step << "  idx = " << idx << " engineStep = " <<            \
           engineStep                                                                              \
                  << "\n";  */                                                                     \
        if (idx < vii.info.size())                                                                 \
        {                                                                                          \
            auto viis = vii.info[idx];                                                             \
            Variable<T> *v = reinterpret_cast<Variable<T> *>(viis.originalVar);                    \
            Engine *e = m_Engines[viis.engineIdx];                                                 \
            auto vec = e->BlocksInfo(*v, engineStep);                                              \
            /* std::cout << "  vec size = " << vec.size() << "\n";  */                             \
            res.insert(res.end(), vec.begin(), vec.end());                                         \
        }                                                                                          \
        return res;                                                                                \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

} // end namespace engine
} // end namespace core
} // end namespace adios2
