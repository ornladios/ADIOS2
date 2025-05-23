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
#include "adios2/helper/adiosNetwork.h"   // GetFQDN
#include "adios2/helper/adiosSystem.h"    // CreateDirectory
#include "adios2/toolkit/remote/EVPathRemote.h"
#include <adios2-perfstubs-interface.h>
#include <adios2sys/SystemTools.hxx>

#include <fstream>
#include <iostream>

#include <nlohmann_json.hpp>

namespace adios2
{
namespace core
{
namespace engine
{

CampaignReader::CampaignReader(IO &io, const std::string &name, const Mode mode, helper::Comm comm)
: Engine("CampaignReader", io, name, mode, std::move(comm))
{
    m_ReaderRank = m_Comm.Rank();
    Init();
    m_IsOpen = true;
}

CampaignReader::~CampaignReader()
{
    if (m_IsOpen)
    {
        DestructorClose(m_FailVerbose);
    }
    m_IsOpen = false;
}

StepStatus CampaignReader::BeginStep(const StepMode mode, const float timeoutSeconds)
{
    // step info should be received from the writer side in BeginStep()
    // so this forced increase should not be here
    ++m_CurrentStep;

    if (m_Options.verbose > 1)
    {
        std::cout << "Campaign Reader " << m_ReaderRank << "   BeginStep() new step "
                  << m_CurrentStep << "\n";
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
    if (m_Options.verbose > 1)
    {
        std::cout << "Campaign Reader " << m_ReaderRank << "     PerformGets()\n";
    }
    for (auto ep : m_Engines)
    {
        ep->PerformGets();
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

    if (m_Options.verbose > 1)
    {
        std::cout << "Campaign Reader " << m_ReaderRank << "   EndStep()\n";
    }
}

// PRIVATE

void CampaignReader::Init()
{
    InitParameters();
    InitTransports();
}

void CampaignReader::InitParameters()
{
    const UserOptions::Campaign &opts = m_UserOptions.campaign;
    m_Options.active = true; // this is really just for Recording
    m_Options.hostname = opts.hostname;
    m_Options.campaignstorepath = opts.campaignstorepath;
    m_Options.cachepath = opts.cachepath;
    m_Options.verbose = opts.verbose;
    for (const auto &pair : m_IO.m_Parameters)
    {
        std::string key(pair.first);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string value(pair.second);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (key == "verbose")
        {
            m_Options.verbose = std::stoi(value);
            if (m_Options.verbose < 0 || m_Options.verbose > 5)
                helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "InitParameters",
                                                     "Method verbose argument must be an "
                                                     "integer in the range [0,5], in call to "
                                                     "Open or Engine constructor");
        }
        if (key == "hostname")
        {
            m_Options.hostname = pair.second;
        }
        if (key == "campaignstorepath")
        {
            m_Options.campaignstorepath = pair.second;
        }
        if (key == "cachepath")
        {
            m_Options.cachepath = pair.second;
        }
    }

    if (m_Options.hostname.empty())
    {
        m_Options.hostname = helper::GetClusterName();
    }

    if (m_Options.verbose > 0)
    {
        std::cout << "CampaignReader: \n";
        std::cout << "  Hostname = " << m_Options.hostname << std::endl;
        std::cout << "  Campaign Store Path = " << m_Options.campaignstorepath << std::endl;
        std::cout << "  Cache Path = " << m_Options.cachepath << std::endl;
    }
}

void CampaignReader::InitTransports()
{
    std::string path = m_Name;
    if (!adios2sys::SystemTools::FileExists(path) && path[0] != '/' && path[0] != '\\' &&
        !m_Options.campaignstorepath.empty())
    {
        std::string path2 = m_Options.campaignstorepath + PathSeparator + m_Name;
        if (adios2sys::SystemTools::FileExists(path2))
        {
            path = path2;
        }
    }

    int rc = sqlite3_open(path.c_str(), &m_DB);
    if (rc)
    {
        std::string dbmsg(sqlite3_errmsg(m_DB));
        sqlite3_close(m_DB);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "Open",
                                             "Cannot open database" + path + ": " + dbmsg);
    }

    ReadCampaignData(m_DB, m_CampaignData);

    if (m_Options.verbose > 0)
    {
        std::cout << "Local hostname = " << m_Options.hostname << "\n";
        std::cout << "Database result:\n  version = " << m_CampaignData.version.version
                  << "\n  hosts:\n";

        for (size_t hostidx = 0; hostidx < m_CampaignData.hosts.size(); ++hostidx)
        {
            CampaignHost &h = m_CampaignData.hosts[hostidx];
            std::cout << "    host = " << h.hostname << "  long name = " << h.longhostname
                      << "  directories: \n";
            for (size_t diridx = 0; diridx < h.dirIdx.size(); ++diridx)
            {
                std::cout << "      dir = " << m_CampaignData.directory[h.dirIdx[diridx]] << "\n";
            }
        }
        std::cout << "  keys:\n";
        for (size_t keyidx = 0; keyidx < m_CampaignData.keys.size(); ++keyidx)
        {
            CampaignKey &k = m_CampaignData.keys[keyidx];
            std::cout << "    key = " << k.id << "\n";
        }
        std::cout << "  datasets:\n";
        for (auto &it : m_CampaignData.datasets)
        {
            CampaignDataset &ds = it.second;
            std::cout << "    " << m_CampaignData.hosts[ds.hostIdx].hostname << ":"
                      << m_CampaignData.directory[ds.dirIdx] << PathSeparator << ds.name << "\n";
            std::cout << "      uuid: " << ds.uuid << "\n";
            std::cout << "      fmt : " << ds.format.ToString() << "\n";
            for (auto &bpf : ds.files)
            {
                std::cout << "      file: " << bpf.name << "\n";
            }
        }
    }

    // std::string cs = m_Comm.BroadcastFile(m_Name, "broadcast campaign file");
    // nlohmann::json js = nlohmann::json::parse(cs);
    // std::cout << "JSON rank " << m_ReaderRank << ": " << js.size() <<
    // std::endl;
    std::unique_ptr<Remote> connectionManager = nullptr;
    int dsIdx = 0;
    for (auto &it : m_CampaignData.datasets)
    {
        ++dsIdx; // Datasets are indexed from 1 in SQL tables
        CampaignDataset &ds = it.second;
        adios2::core::IO &io = m_IO.m_ADIOS.DeclareIO("CampaignReader" + std::to_string(dsIdx));
        std::string localPath;
        if (m_CampaignData.hosts[ds.hostIdx].hostname != m_Options.hostname)
        {
            bool done = false;
            auto it = m_HostOptions.find(m_CampaignData.hosts[ds.hostIdx].hostname);
            if (it != m_HostOptions.end())
            {
                const HostConfig &ho = (it->second).front();
                if (ho.protocol == HostAccessProtocol::S3)
                {
                    const std::string endpointURL = ho.endpoint;
                    const std::string objPath = m_CampaignData.directory[ds.dirIdx] + "/" + ds.name;
                    Params p;
                    p.emplace("Library", "awssdk");
                    p.emplace("endpoint", endpointURL);
                    p.emplace("cache", m_Options.cachepath + PathSeparator +
                                           m_CampaignData.hosts[ds.hostIdx].hostname +
                                           PathSeparator + m_Name);
                    p.emplace("verbose", std::to_string(ho.verbose));
                    p.emplace("recheck_metadata", (ho.recheckMetadata ? "true" : "false"));
                    io.AddTransport("File", p);
                    io.SetEngine("BP5");
                    localPath = m_CampaignData.directory[ds.dirIdx] + PathSeparator + ds.name;
                    if (ho.isAWS_EC2)
                    {
                        adios2sys::SystemTools::PutEnv("AWS_EC2_METADATA_DISABLED=false");
                    }
                    else
                    {
                        adios2sys::SystemTools::PutEnv("AWS_EC2_METADATA_DISABLED=true");
                    }

                    if (ho.awsProfile.empty())
                    {
                        adios2sys::SystemTools::PutEnv("AWS_PROFILE=default");
                    }
                    else
                    {
                        std::string es = "AWS_PROFILE=" + ho.awsProfile;
                        adios2sys::SystemTools::PutEnv(es);
                    }

                    done = true;
                }
            }

            if (!done)
            {
                const std::string remotePath =
                    m_CampaignData.directory[ds.dirIdx] + PathSeparator + ds.name;
                const std::string remoteURL =
                    m_CampaignData.hosts[ds.hostIdx].hostname + ":" + remotePath;
                localPath = m_Options.cachepath + PathSeparator + ds.uuid.substr(0, 3) +
                            PathSeparator + ds.uuid;
                if (m_Options.verbose > 0)
                {
                    std::cout << "Open remote file " << remoteURL << " \n";
                }

                std::string keyhex;
                if (ds.hasKey)
                {
                    if (m_Options.verbose > 0)
                    {
                        std::cout << "The dataset is key protected with key id "
                                  << m_CampaignData.keys[ds.keyIdx].id << "\n";
                    }
#ifdef ADIOS2_HAVE_SODIUM
                    if (m_CampaignData.keys[ds.keyIdx].keyHex.empty())
                    {
                        // Retrieve key
                        if (!connectionManager)
                        {
                            connectionManager = std::unique_ptr<Remote>(
                                new Remote(core::ADIOS::StaticGetHostOptions()));
                        }
                        m_CampaignData.keys[ds.keyIdx].keyHex =
                            connectionManager->GetKeyFromConnectionManager(
                                m_CampaignData.keys[ds.keyIdx].id);

                        if (m_Options.verbose > 0)
                        {
                            std::cout << "-- Received key " << m_CampaignData.keys[ds.keyIdx].keyHex
                                      << "\n";
                        }
                    }

                    if (m_CampaignData.keys[ds.keyIdx].keyHex == "0")
                    {
                        // We received no key, ignore files encrypted with this key
                        std::cerr << "ERROR: don't have the key "
                                  << m_CampaignData.keys[ds.keyIdx].id << " to decrypt " << ds.name
                                  << ". Ignoring this dataset." << std::endl;
                        continue;
                    }

                    keyhex = m_CampaignData.keys[ds.keyIdx].keyHex;
#else
                    helper::Throw<std::runtime_error>(
                        "Engine", "CampaignReader", "InitTransports",
                        "ADIOS needs to be built with libsodium and with SST to "
                        "be able to process protected campaign files");
#endif
                }

                if (ds.format != FileFormat::TEXT)
                {
                    if (m_Options.verbose > 0)
                    {
                        std::cout << "    use local cache for metadata at " << localPath << " \n";
                    }
                    helper::CreateDirectory(localPath);
                }

                // first file is HDF5's file to be opened as path
                // second file is min/max data if available
                // first file if TEXT's file to be opened as path
                std::string newLocalPath = localPath;
                bool setHDF5FilePath = true;
                for (auto &f : ds.files)
                {
                    std::string path =
                        localPath + PathSeparator + adios2sys::SystemTools::GetFilenameName(f.name);
                    if (ds.format != FileFormat::TEXT)
                    {
                        SaveToFile(m_DB, path, f, keyhex, m_CampaignData);
                    }
                    else
                    {
                        // TEXT -> create a variable, will read from DB directly, no local path
                        CreateTextVariable(f.name, f.lengthOriginal, dsIdx);
                        continue;
                    }

                    if (setHDF5FilePath)
                    {
                        if (ds.format == FileFormat::HDF5)
                        {
                            newLocalPath = path;
                            setHDF5FilePath = false;
                        }
                    }
                }

                io.SetParameter("RemoteDataPath", remotePath);
                io.SetParameter("RemoteHost", m_CampaignData.hosts[ds.hostIdx].hostname);
                io.SetParameter("UUID", ds.uuid);

                // Save info in cache directory for cache manager and for humans
                {
                    std::ofstream f(localPath + PathSeparator + "info.txt");
                    if (f.is_open())
                    {
                        f << "Campaign = " << m_Name << "\n";
                        f << "Dataset = " << ds.name << "\n";
                        f << "RemoteHost = " << m_CampaignData.hosts[ds.hostIdx].hostname << "\n";
                        f << "RemoteDataPath = " << remotePath << "\n";
                        f.close();
                    }
                }

                // HDF5/TEXT point to the first file under the uuid folder in cache
                localPath = newLocalPath;
            }
        }
        else
        {
            localPath = m_CampaignData.directory[ds.dirIdx] + PathSeparator + ds.name;
            if (m_Options.verbose > 0)
            {
                std::cout << "Open local file " << localPath << "\n";
            }
            if (ds.format == FileFormat::TEXT)
            {
                // TEXT -> create a variable
                CreateTextVariable(ds.name, adios2sys::SystemTools::FileLength(localPath), dsIdx);
            }
        }

        if (ds.format == FileFormat::HDF5)
        {
            io.SetEngine("HDF5");
        }
        else if (ds.format == FileFormat::TEXT)
        {
            continue;
        }
        else if (ds.format == FileFormat::Unknown)
        {
            continue;
        }

        adios2::core::Engine &e = io.Open(localPath, m_OpenMode, m_Comm.Duplicate());

        m_IOs.push_back(&io);
        m_Engines.push_back(&e);

        auto vmap = io.GetAvailableVariables();
        auto amap = io.GetAvailableAttributes();
        VarInternalInfo internalInfo(nullptr, m_IOs.size() - 1, m_Engines.size() - 1);

        for (auto &vr : vmap)
        {
            auto vname = vr.first;
            std::string fname = ds.name;
            std::string newname;
            if (ds.format == FileFormat::HDF5)
            {
                newname = fname + vname;
            }
            else
            {
                newname = fname + "/" + vname;
            }

            const DataType type = io.InquireVariableType(vname);

            if (type == DataType::Struct)
            {
            }
#define declare_type(T)                                                                            \
    else if (type == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        Variable<T> *vi = io.InquireVariable<T>(vname);                                            \
        Variable<T> v = DuplicateVariable(vi, m_IO, newname, internalInfo);                        \
    }

            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        }

        for (auto &ar : amap)
        {
            auto aname = ar.first;
            std::string fname = ds.name;
            std::string newname = fname + "/" + aname;

            const DataType type = io.InquireAttributeType(aname);

            if (type == DataType::Struct)
            {
            }
#define declare_type(T)                                                                            \
    else if (type == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        Attribute<T> *ai = io.InquireAttribute<T>(aname);                                          \
        Attribute<T> v = DuplicateAttribute(ai, m_IO, newname);                                    \
    }

            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        }
    }
}

void CampaignReader::DoClose(const int transportIndex)
{
    if (m_Options.verbose > 1)
    {
        std::cout << "Campaign Reader " << m_ReaderRank << " Close(" << m_Name << ")\n";
    }
    for (auto ep : m_Engines)
    {
        ep->Close();
    }
    sqlite3_close(m_DB);
    m_IsOpen = false;
}

void CampaignReader::CreateTextVariable(const std::string &name, const size_t len,
                                        const size_t dsIdx)
{
    // TEXT -> create a variable, will read from DB directly, no local path
    Variable<char> &v = m_IO.DefineVariable<char>(name, Dims{len}, Dims{0ULL}, Dims{len}, false);
    v.m_AvailableStepsCount = 1;
    v.m_AvailableStepsStart = 0;
    CampaignVarInternalInfo internalInfo(&v, dsIdx);
    m_CampaignVarInternalInfo.emplace(v.m_Name, internalInfo);
}

void CampaignReader::DestructorClose(bool Verbose) noexcept { sqlite3_close(m_DB); }

// Remove the engine name from the var name, which must be of pattern
// <engineName>/<original var name>
/*static std::string RemoveEngineName(const std::string &varName,
                                    const std::string &engineName)
{
    auto le = engineName.size() + 1;
    auto v = varName.substr(le);
    return v;
}*/

MinVarInfo *CampaignReader::MinBlocksInfo(const VariableBase &Var, size_t Step) const
{
    auto it = m_VarInternalInfo.find(Var.m_Name);
    if (it != m_VarInternalInfo.end())
    {
        VariableBase *vb = reinterpret_cast<VariableBase *>(it->second.originalVar);
        Engine *e = m_Engines[it->second.engineIdx];
        MinVarInfo *MV = e->MinBlocksInfo(*vb, Step);
        if (MV)
        {
            return MV;
        }
    }
    else
    {
        auto it = m_CampaignVarInternalInfo.find(Var.m_Name);
        if (it != m_CampaignVarInternalInfo.end())
        {
            int ndims = static_cast<int>(Var.m_Shape.size());
            MinVarInfo *MV = new MinVarInfo(ndims, Var.m_Shape.data());
            MV->Step = 0;
            MV->Dims = ndims;
            MV->Shape = Var.m_Shape.data();
            MV->WasLocalValue = false;
            MV->IsValue = false;
            MV->IsReverseDims = false;
            MV->BlocksInfo.reserve(1);
            MinBlockInfo Blk;
            Blk.WriterID = 0;
            Blk.BlockID = 0;
            Blk.Start = Var.m_Start.data();
            Blk.Count = Var.m_Count.data();
            Blk.MinMax.Init(adios2::DataType::Char);
            Blk.MinMax.MinUnion.field_int8 = (int8_t)'a';
            Blk.MinMax.MaxUnion.field_int8 = (int8_t)'z';
            MV->BlocksInfo.push_back(Blk);
            return MV;
        }
    }
    return nullptr;
}

bool CampaignReader::VarShape(const VariableBase &Var, const size_t Step, Dims &Shape) const
{
    auto it = m_VarInternalInfo.find(Var.m_Name);
    if (it != m_VarInternalInfo.end())
    {
        VariableBase *vb = reinterpret_cast<VariableBase *>(it->second.originalVar);
        Engine *e = m_Engines[it->second.engineIdx];
        return e->VarShape(*vb, Step, Shape);
    }
    else
    {
        auto it = m_CampaignVarInternalInfo.find(Var.m_Name);
        if (it != m_CampaignVarInternalInfo.end())
        {
            Shape = Var.m_Shape;
            return true;
        }
    }
    return false;
}

bool CampaignReader::VariableMinMax(const VariableBase &Var, const size_t Step,
                                    MinMaxStruct &MinMax)
{
    auto it = m_VarInternalInfo.find(Var.m_Name);
    if (it != m_VarInternalInfo.end())
    {
        VariableBase *vb = reinterpret_cast<VariableBase *>(it->second.originalVar);
        Engine *e = m_Engines[it->second.engineIdx];
        return e->VariableMinMax(*vb, Step, MinMax);
    }
    else
    {
        auto it = m_CampaignVarInternalInfo.find(Var.m_Name);
        if (it != m_CampaignVarInternalInfo.end())
        {
            MinMax.Init(adios2::DataType::Char);
            MinMax.MinUnion.field_int8 = (int8_t)'a';
            MinMax.MaxUnion.field_int8 = (int8_t)'z';
            return true;
        }
    }
    return false;
}

std::string CampaignReader::VariableExprStr(const VariableBase &Var)
{
    auto it = m_VarInternalInfo.find(Var.m_Name);
    if (it != m_VarInternalInfo.end())
    {
        VariableBase *vb = reinterpret_cast<VariableBase *>(it->second.originalVar);
        Engine *e = m_Engines[it->second.engineIdx];
        return e->VariableExprStr(*vb);
    }
    return "";
}

#define declare_type(T)                                                                            \
    void CampaignReader::DoGetSync(Variable<T> &variable, T *data) { GetSyncTCC(variable, data); } \
    void CampaignReader::DoGetDeferred(Variable<T> &variable, T *data)                             \
    {                                                                                              \
        GetDeferredTCC(variable, data);                                                            \
    }                                                                                              \
                                                                                                   \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>>                                    \
    CampaignReader::DoAllStepsBlocksInfo(const Variable<T> &variable) const                        \
    {                                                                                              \
        PERFSTUBS_SCOPED_TIMER("CampaignReader::AllStepsBlocksInfo");                              \
        auto it = m_VarInternalInfo.find(variable.m_Name);                                         \
        if (it == m_VarInternalInfo.end())                                                         \
        {                                                                                          \
            std::map<size_t, std::vector<typename core::Variable<T>::BPInfo>> allStepsBlocksInfo;  \
            /*allStepsBlocksInfo[0] = ;*/                                                          \
            return allStepsBlocksInfo;                                                             \
        }                                                                                          \
        Variable<T> *v = reinterpret_cast<Variable<T> *>(it->second.originalVar);                  \
        Engine *e = m_Engines[it->second.engineIdx];                                               \
        return e->AllStepsBlocksInfo(*v);                                                          \
    }                                                                                              \
                                                                                                   \
    std::vector<std::vector<typename Variable<T>::BPInfo>>                                         \
    CampaignReader::DoAllRelativeStepsBlocksInfo(const Variable<T> &variable) const                \
    {                                                                                              \
        PERFSTUBS_SCOPED_TIMER("CampaignReader::AllRelativeStepsBlocksInfo");                      \
        auto it = m_VarInternalInfo.find(variable.m_Name);                                         \
        Variable<T> *v = reinterpret_cast<Variable<T> *>(it->second.originalVar);                  \
        Engine *e = m_Engines[it->second.engineIdx];                                               \
        return e->AllRelativeStepsBlocksInfo(*v);                                                  \
    }                                                                                              \
                                                                                                   \
    std::vector<typename Variable<T>::BPInfo> CampaignReader::DoBlocksInfo(                        \
        const Variable<T> &variable, const size_t step) const                                      \
    {                                                                                              \
        PERFSTUBS_SCOPED_TIMER("CampaignReader::BlocksInfo");                                      \
        auto it = m_VarInternalInfo.find(variable.m_Name);                                         \
        Variable<T> *v = reinterpret_cast<Variable<T> *>(it->second.originalVar);                  \
        Engine *e = m_Engines[it->second.engineIdx];                                               \
        return e->BlocksInfo(*v, step);                                                            \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void CampaignReader::GetVariableFromDB(std::string name, size_t dsIdx, DataType type, void *data)
{
    if (type != DataType::Char)
    {
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "GetVariableFromDB",
                                             "Expected char variable but was called with a type " +
                                                 ToString(type));
    }

    CampaignDataset &ds = m_CampaignData.datasets[dsIdx];
    const CampaignFile &f = ds.files[0];
    std::string keyhex;
    if (ds.hasKey)
        keyhex = m_CampaignData.keys[ds.keyIdx].keyHex;
    ReadToMemory(m_DB, (char *)data, f, keyhex, m_CampaignData);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
