/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignReader.cpp
 *
 *  Created on: May 15, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "CampaignReader.h"
#include "CampaignReader.tcc"

#include "adios2/helper/adiosFunctions.h" // CSVToVector
#include <adios2-perfstubs-interface.h>

#include <fstream>
#include <iostream>

#include <nlohmann_json.hpp>

namespace adios2
{
namespace core
{
namespace engine
{

CampaignReader::CampaignReader(IO &io, const std::string &name, const Mode mode,
                               helper::Comm comm)
: Engine("CampaignReader", io, name, mode, std::move(comm))
{
    m_ReaderRank = m_Comm.Rank();
    Init();
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Reader " << m_ReaderRank << " Open(" << m_Name
                  << ") in constructor." << std::endl;
    }
    m_IsOpen = true;
}

CampaignReader::~CampaignReader()
{
    /* CampaignReader destructor does close and finalize */
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Reader " << m_ReaderRank << " deconstructor on "
                  << m_Name << "\n";
    }
    if (m_IsOpen)
    {
        DestructorClose(m_FailVerbose);
    }
    m_IsOpen = false;
}

StepStatus CampaignReader::BeginStep(const StepMode mode,
                                     const float timeoutSeconds)
{
    // step info should be received from the writer side in BeginStep()
    // so this forced increase should not be here
    ++m_CurrentStep;

    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Reader " << m_ReaderRank
                  << "   BeginStep() new step " << m_CurrentStep << "\n";
    }

    // If we reach the end of stream (writer is gone or explicitly tells the
    // reader)
    // we return EndOfStream to the reader application
    if (m_CurrentStep == 2)
    {
        std::cout << "Campaign Reader " << m_ReaderRank
                  << "   forcefully returns End of Stream at this step\n";

        return StepStatus::EndOfStream;
    }

    // We should block until a new step arrives or reach the timeout

    // m_IO Variables and Attributes should be defined at this point
    // so that the application can inquire them and start getting data

    return StepStatus::OK;
}

void CampaignReader::PerformGets()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Reader " << m_ReaderRank
                  << "     PerformGets()\n";
    }
    m_NeedPerformGets = false;
}

size_t CampaignReader::CurrentStep() const { return m_CurrentStep; }

void CampaignReader::EndStep()
{
    // EndStep should call PerformGets() if there are unserved GetDeferred()
    // requests
    if (m_NeedPerformGets)
    {
        PerformGets();
    }

    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Reader " << m_ReaderRank << "   EndStep()\n";
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void CampaignReader::DoGetSync(Variable<T> &variable, T *data)             \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void CampaignReader::DoGetDeferred(Variable<T> &variable, T *data)         \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void CampaignReader::Init()
{
    InitParameters();
    InitTransports();
}

void CampaignReader::InitParameters()
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
                helper::Throw<std::invalid_argument>(
                    "Engine", "CampaignReader", "InitParameters",
                    "Method verbose argument must be an "
                    "integer in the range [0,5], in call to "
                    "Open or Engine constructor");
        }
    }
}

void CampaignReader::InitTransports()
{
    std::string cs = m_Comm.BroadcastFile(m_Name, "broadcast campaign file");
    nlohmann::json js = nlohmann::json::parse(cs);
    std::cout << "JSON rank " << m_ReaderRank << ": " << js.size() << std::endl;
    int i = 0;
    for (auto &jf : js)
    {
        std::cout << jf << std::endl;
        adios2::core::IO &io =
            m_IO.m_ADIOS.DeclareIO("CampaignReader" + std::to_string(i));
        adios2::core::Engine &e =
            io.Open(jf["name"], m_OpenMode, m_Comm.Duplicate());

        m_IOs.push_back(&io);
        m_Engines.push_back(&e);

        auto vmap = io.GetAvailableVariables();
        auto amap = io.GetAvailableAttributes();
        VarInternalInfo internalInfo(nullptr, m_IOs.size() - 1,
                                     m_Engines.size() - 1);

        for (auto &vr : vmap)
        {
            auto vname = vr.first;
            std::string fname = jf["name"];
            std::string newname = fname + "/" + vname;

            const DataType type = io.InquireVariableType(vname);

            if (type == DataType::Struct)
            {
            }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        Variable<T> *vi = io.InquireVariable<T>(vname);                        \
        Variable<T> v =                                                        \
            DuplicateVariable(vi, m_IO, newname, &e, internalInfo);            \
    }

            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        }

        ++i;
    }
}

void CampaignReader::DoClose(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Campaign Reader " << m_ReaderRank << " Close(" << m_Name
                  << ")\n";
    }
    for (auto ep : m_Engines)
    {
        ep->Close();
    }
}

// Remove the engine name from the var name, which must be of pattern
// <engineName>/<original var name>
/*static std::string RemoveEngineName(const std::string &varName,
                                    const std::string &engineName)
{
    auto le = engineName.size() + 1;
    auto v = varName.substr(le);
    return v;
}*/

MinVarInfo *CampaignReader::MinBlocksInfo(const VariableBase &Var,
                                          size_t Step) const
{
    auto it = m_VarInternalInfo.find(Var.m_Name);
    if (it != m_VarInternalInfo.end())
    {
        VariableBase *vb =
            reinterpret_cast<VariableBase *>(it->second.originalVar);
        Engine *e = m_Engines[it->second.engineIdx];
        MinVarInfo *MV = e->MinBlocksInfo(*vb, Step);
        if (MV)
        {
            return MV;
        }
    }
    return nullptr;
}

#define declare_type(T)                                                        \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>>                \
    CampaignReader::DoAllStepsBlocksInfo(const Variable<T> &variable) const    \
    {                                                                          \
        PERFSTUBS_SCOPED_TIMER("CampaignReader::AllStepsBlocksInfo");          \
        auto it = m_VarInternalInfo.find(variable.m_Name);                     \
        Variable<T> *v =                                                       \
            reinterpret_cast<Variable<T> *>(it->second.originalVar);           \
        Engine *e = m_Engines[it->second.engineIdx];                           \
        return e->AllStepsBlocksInfo(*v);                                      \
    }                                                                          \
                                                                               \
    std::vector<std::vector<typename Variable<T>::BPInfo>>                     \
    CampaignReader::DoAllRelativeStepsBlocksInfo(const Variable<T> &variable)  \
        const                                                                  \
    {                                                                          \
        PERFSTUBS_SCOPED_TIMER("CampaignReader::AllRelativeStepsBlocksInfo");  \
        auto it = m_VarInternalInfo.find(variable.m_Name);                     \
        Variable<T> *v =                                                       \
            reinterpret_cast<Variable<T> *>(it->second.originalVar);           \
        Engine *e = m_Engines[it->second.engineIdx];                           \
        return e->AllRelativeStepsBlocksInfo(variable);                        \
    }                                                                          \
                                                                               \
    std::vector<typename Variable<T>::BPInfo> CampaignReader::DoBlocksInfo(    \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        PERFSTUBS_SCOPED_TIMER("CampaignReader::BlocksInfo");                  \
        auto it = m_VarInternalInfo.find(variable.m_Name);                     \
        Variable<T> *v =                                                       \
            reinterpret_cast<Variable<T> *>(it->second.originalVar);           \
        Engine *e = m_Engines[it->second.engineIdx];                           \
        return e->BlocksInfo(variable, step);                                  \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

} // end namespace engine
} // end namespace core
} // end namespace adios2