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
#include "adios2/toolkit/remote/XrootdRemote.h"
#include <adios2-perfstubs-interface.h>
#include <adios2sys/SystemTools.hxx>

#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>

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

    size_t nextEngine = 0;
    size_t nEngines = m_Engines.size();
    std::mutex mutexNext;

    auto lf_GetNext = [&]() -> size_t {
        std::lock_guard<std::mutex> lockGuard(mutexNext);
        size_t reqidx = MaxSizeT;
        if (nextEngine < nEngines)
        {
            reqidx = nextEngine;
            ++nextEngine;
        }
        return reqidx;
    };

    auto lf_WaitForPerformGets = [&](const size_t threadID) -> bool {
        while (true)
        {
            const auto engineIdx = lf_GetNext();
            if (engineIdx > nEngines)
            {
                break;
            }
            m_Engines[engineIdx]->PerformGets();
        }
        return true;
    };

    size_t nThreads = std::min(nEngines, (size_t)16);
    std::vector<std::future<bool>> futures(nThreads - 1);

    // launch Threads-1 threads to process subsets of handles,
    // then main thread process the last subset
    for (size_t tid = 0; tid < nThreads - 1; ++tid)
    {
        futures[tid] = std::async(std::launch::async, lf_WaitForPerformGets, tid + 1);
    }

    // main thread runs last subset of reads
    lf_WaitForPerformGets(0);

    // wait for all async threads
    for (auto &f : futures)
    {
        f.get();
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
    m_Options.active = false; // this is really just for Recording
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
        else if (key == "hostname")
        {
            m_Options.hostname = pair.second;
        }
        else if (key == "campaignstorepath")
        {
            m_Options.campaignstorepath = pair.second;
        }
        else if (key == "cachepath")
        {
            m_Options.cachepath = pair.second;
        }
        else if (key == "include-dataset")
        {
            m_IncludePatterns = helper::StringToVector(pair.second, ';');
            for (auto &s : m_IncludePatterns)
            {
                m_IncludePatternsRe.push_back(std::regex(s));
            }
        }
        else if (key == "exclude-dataset")
        {
            m_ExcludePatterns = helper::StringToVector(pair.second, ';');
            for (auto &s : m_ExcludePatterns)
            {
                m_ExcludePatternsRe.push_back(std::regex(s));
            }
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
        if (!m_IncludePatterns.empty())
        {
            std::cout << "  Include patterns = [";
            for (size_t idx = 0; idx < m_IncludePatterns.size(); ++idx)
            {
                std::cout << m_IncludePatterns[idx];
                if (idx < m_IncludePatterns.size() - 1)
                    std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }
        if (!m_ExcludePatterns.empty())
        {
            std::cout << "  Exclude patterns = [";
            for (size_t idx = 0; idx < m_ExcludePatterns.size(); ++idx)
            {
                std::cout << m_ExcludePatterns[idx];
                if (idx < m_ExcludePatterns.size() - 1)
                    std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }
    }
}

std::string CampaignReader::SaveRemoteMD(size_t dsIdx, size_t repIdx, adios2::core::IO &io)
{
    std::string localPath, cachePath; // same for BP, but for HDF5 localpath=cachepath/<filename>
    CampaignDataset &ds = m_CampaignData.datasets[dsIdx];
    CampaignReplica &rep = ds.replicas[repIdx];

    std::string tarpath;
    if (rep.archiveIdx > 0)
    {
        auto itTarName = m_CampaignData.tarnames.find(rep.archiveIdx);
        if (itTarName != m_CampaignData.tarnames.end())
        {
            tarpath = itTarName->second + "/";
        }
    }

    const std::string remotePath =
        m_CampaignData.directory[rep.dirIdx].path + "/" + tarpath + rep.name;
    const std::string remoteURL = m_CampaignData.hosts[rep.hostIdx].hostname + ":" + remotePath;
    localPath =
        m_Options.cachepath + PathSeparator + ds.uuid.substr(0, 3) + PathSeparator + ds.uuid;
    cachePath = localPath;
    if (m_Options.verbose > 0)
    {
        std::cout << "Open remote file " << remoteURL << " \n";
    }

    std::string keyhex;
    if (rep.hasKey)
    {
        if (m_Options.verbose > 0)
        {
            std::cout << "The dataset is key protected with key id "
                      << m_CampaignData.keys[rep.keyIdx].id << "\n";
        }
#ifdef ADIOS2_HAVE_SODIUM
        if (m_CampaignData.keys[rep.keyIdx].keyHex.empty())
        {
            // Retrieve key
            if (!m_ConnectionManager)
            {
                m_ConnectionManager =
                    std::unique_ptr<Remote>(new Remote(core::ADIOS::StaticGetHostOptions()));
            }
            m_CampaignData.keys[rep.keyIdx].keyHex =
                m_ConnectionManager->GetKeyFromConnectionManager(
                    m_CampaignData.keys[rep.keyIdx].id);

            if (m_Options.verbose > 0)
            {
                std::cout << "-- Received key " << m_CampaignData.keys[rep.keyIdx].keyHex << "\n";
            }
        }

        if (m_CampaignData.keys[rep.keyIdx].keyHex == "0")
        {
            // We received no key, ignore files encrypted with this key
            std::cerr << "ERROR: don't have the key " << m_CampaignData.keys[rep.keyIdx].id
                      << " to decrypt " << ds.name << ". Ignoring this dataset." << std::endl;
            return "";
        }

        keyhex = m_CampaignData.keys[rep.keyIdx].keyHex;
#else
        helper::Throw<std::runtime_error>("Engine", "CampaignReader", "InitTransports",
                                          "ADIOS needs to be built with libsodium and with SST to "
                                          "be able to process protected campaign files");
#endif
    }

    if (ds.format != FileFormat::TEXT)
    {
        if (m_Options.verbose > 0)
        {
            std::cout << "    use local cache for metadata at " << cachePath << " \n";
        }
        helper::CreateDirectory(cachePath);
    }

    // first file is HDF5's file to be opened as path
    // second file is min/max data if available
    // first file if TEXT's file to be opened as path
    std::string newLocalPath = localPath;
    bool setHDF5FilePath = true;
    if (rep.files.size())
    {
        for (auto &f : rep.files)
        {
            std::string path =
                localPath + PathSeparator + adios2sys::SystemTools::GetFilenameName(f.name);
            if (ds.format == FileFormat::TEXT)
            {
                // TEXT -> create a variable, will read from DB directly, no local path
                CreateTextVariable(ds.name, f.lengthOriginal, dsIdx, repIdx);
                continue;
            }
            else if (ds.format == FileFormat::IMAGE)
            {
                // IMAGE -> create a variable, will read from DB directly, no local path
                std::string imgName =
                    ds.name + "/" + std::to_string(rep.x) + "x" + std::to_string(rep.y);
                CreateImageVariable(imgName, f.lengthOriginal, dsIdx, repIdx);
                continue;
            }
            else
            {
                m_CampaignData.SaveToFile(path, f, keyhex);
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
    }
    else
    {
        // a remote dataset without any embedded file
        if (ds.format == FileFormat::TEXT)
        {
            // TEXT -> create a variable, will read from remote
            CreateTextVariable(ds.name, rep.size, dsIdx, repIdx);
        }
        else if (ds.format == FileFormat::IMAGE)
        {
            // IMAGE -> create a variable, will read from remote
            std::string imgName =
                ds.name + "/" + std::to_string(rep.x) + "x" + std::to_string(rep.y);
            CreateImageVariable(imgName, rep.size, dsIdx, repIdx);
        }
        else
        {
            helper::Throw<std::invalid_argument>(
                "Engine", "CampaignReader", "Open",
                "ADIOS/HDF5 replicas must have embedded metadata file(s) but found none for " +
                    rep.name);
        }
    }

    // Handle https and s3 differently from other host accesses
    if (!m_CampaignData.hosts[rep.hostIdx].defaultProtocol.empty())
    {
        // https or s3 protocols handled here
        if (m_CampaignData.hosts[rep.hostIdx].defaultProtocol == "s3")
        {
            auto it = m_HostOptions.find(m_CampaignData.hosts[rep.hostIdx].hostname);
            if (it != m_HostOptions.end())
            {
                const HostConfig &ho = (it->second).front();
                if (ho.protocol == HostAccessProtocol::S3)
                {
                    const std::string endpointURL = ho.endpoint;
                    const std::string objPath =
                        m_CampaignData.directory[rep.dirIdx].path + "/" + rep.name;
                    Params p;
                    p.emplace("Library", "awssdk");
                    p.emplace("endpoint", endpointURL);
                    p.emplace("cache", cachePath);
                    p.emplace("verbose", std::to_string(ho.verbose));
                    p.emplace("recheck_metadata", (ho.recheckMetadata ? "true" : "false"));
                    io.AddTransport("File", p);
                    io.SetParameter("UUID", ds.uuid);
                    io.SetEngine("BP5");
                    newLocalPath = remotePath;
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
                }
            }
        }
        else if (m_CampaignData.hosts[rep.hostIdx].defaultProtocol == "https")
        {
            std::string url = "https://" + m_CampaignData.hosts[rep.hostIdx].longhostname + "/" +
                              m_CampaignData.directory[rep.dirIdx].path + "/" + tarpath + rep.name;
            Params p;
            p.emplace("Library", "https");
            // p.emplace("hostname", m_CampaignData.hosts[rep.hostIdx].longhostname);
            // p.emplace("path", m_CampaignData.directory[rep.dirIdx].path + "/" + tarpath +
            // rep.name);
            p.emplace("cache", cachePath);
            p.emplace("verbose", std::to_string(m_Options.verbose));
            p.emplace("recheck_metadata", "false");
            io.AddTransport("File", p);
            io.SetParameter("UUID", ds.uuid);
            if (ds.format == FileFormat::HDF5)
            {
                io.SetEngine("HDF5");
            }
            else
            {
                io.SetEngine("BP5");
            }

            if (m_Options.verbose > 0)
            {
                std::cout << "Open remote URL " << url << " \n";
            }
            newLocalPath = url;
        }
    }
    else
    {
        io.SetParameter("RemoteDataPath", remotePath);
        io.SetParameter("RemoteHost", m_CampaignData.hosts[rep.hostIdx].hostname);
        io.SetParameter("UUID", ds.uuid);
    }
    // Save info in cache directory for cache manager and for humans
    {
        std::ofstream f(localPath + PathSeparator + "info.txt");
        if (f.is_open())
        {
            f << "Campaign = " << m_Name << "\n";
            f << "Dataset = " << ds.name << "\n";
            f << "Replica idx = " << repIdx << "\n";
            f << "Replica = " << rep.name << "\n";
            f << "RemoteHost = " << m_CampaignData.hosts[rep.hostIdx].hostname << "\n";
            f << "RemoteDataPath = " << remotePath << "\n";
            f.close();
        }
    }

    // HDF5/TEXT point to the first file under the uuid folder in cache
    localPath = newLocalPath;

    return localPath;
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

    m_CampaignData.Open(path);
    m_CampaignData.ReadDatabase();

    if (m_Options.verbose > 0)
    {
        std::cout << "Local hostname = " << m_Options.hostname << "\n";
        std::cout << "Database result:\n  version = " << m_CampaignData.version.version
                  << "\n  hosts:\n";

        for (size_t hostidx = 0; hostidx < m_CampaignData.hosts.size(); ++hostidx)
        {
            CampaignHost &h = m_CampaignData.hosts[hostidx];
            std::cout << "    " << hostidx + 1 << ". host = " << h.hostname
                      << "  long name = " << h.longhostname << "  directories: \n";
            for (size_t diridx = 0; diridx < h.dirIdx.size(); ++diridx)
            {
                std::cout << "      " << h.dirIdx[diridx] + 1
                          << ". dir = " << m_CampaignData.directory[h.dirIdx[diridx]].path;
                if (m_CampaignData.directory[h.dirIdx[diridx]].archive)
                {
                    std::cout << "  - Archive: "
                              << m_CampaignData.directory[h.dirIdx[diridx]].archiveSystemName;
                    for (size_t archiveidx : m_CampaignData.directory[h.dirIdx[diridx]].archiveIDs)
                    {
                        std::cout << "\n        " << h.dirIdx[diridx] + 1 << "." << archiveidx
                                  << " " << m_CampaignData.tarnames[archiveidx];
                    }
                }
                std::cout << "\n";
            }
        }
        std::cout << "  keys:\n";
        for (size_t keyidx = 0; keyidx < m_CampaignData.keys.size(); ++keyidx)
        {
            CampaignKey &k = m_CampaignData.keys[keyidx];
            std::cout << "    key = " << k.id << "\n";
        }
        std::cout << "  datasets:\n";
        for (auto &itDS : m_CampaignData.datasets)
        {
            CampaignDataset &ds = itDS.second;
            if (ds.tsid && (ds.format == FileFormat::HDF5 || ds.format == FileFormat::ADIOS))
                continue;
            std::cout << "    " << ds.name << "\n";
            std::cout << "      uuid: " << ds.uuid << "\n";
            std::cout << "      fmt : " << ds.format.ToString() << "\n";
            for (auto &itRep : ds.replicas)
            {
                CampaignReplica &rep = itRep.second;
                std::string tarpath;
                if (rep.archiveIdx > 0)
                {
                    auto itTarName = m_CampaignData.tarnames.find(rep.archiveIdx);
                    if (itTarName != m_CampaignData.tarnames.end())
                    {
                        tarpath = itTarName->second + "/";
                    }
                }
                std::cout << "      " << m_CampaignData.hosts[rep.hostIdx].hostname << ":"
                          << m_CampaignData.directory[rep.dirIdx].path << "/" << tarpath << rep.name
                          << "\n";
                if (rep.hasKey)
                {
                    std::cout << "          key: " << rep.keyIdx << "\n";
                }
                for (auto &f : rep.files)
                {
                    std::cout << "          file: " << f.name << "\n";
                }
            }
        }
        std::cout << "\n\n  time series:\n";
        for (auto &itTS : m_CampaignData.timeseries)
        {
            size_t tsid = itTS.first;
            CampaignTimeSeries &ts = itTS.second;
            auto &ds = m_CampaignData.datasets[ts.datasets.begin()->second];
            if (ds.format == FileFormat::IMAGE || ds.format == FileFormat::TEXT)
                continue;
            std::cout << "    " << tsid << ". " << ts.name << "\n";
            for (auto &itDS : ts.datasets)
            {
                size_t tsorder = itDS.first;
                size_t dsid = itDS.second;
                CampaignDataset &ds = m_CampaignData.datasets[dsid];
                std::cout << "    " << ds.name << "\n";
                std::cout << "      tsorder: " << tsorder << "\n";
                std::cout << "      uuid: " << ds.uuid << "\n";
                std::cout << "      fmt : " << ds.format.ToString() << "\n";
                for (auto &itRep : ds.replicas)
                {
                    CampaignReplica &rep = itRep.second;
                    std::cout << "      " << m_CampaignData.hosts[rep.hostIdx].hostname << ":"
                              << m_CampaignData.directory[rep.dirIdx].path << "/" << rep.name
                              << "\n";
                    if (rep.hasKey)
                    {
                        std::cout << "          key: " << rep.keyIdx << "\n";
                    }
                    for (auto &f : rep.files)
                    {
                        std::cout << "          file: " << f.name << "\n";
                    }
                }
            }
        }
    }

    // process ADIOS/HDF5 datasets per time-series first
    for (auto &itTS : m_CampaignData.timeseries)
    {
        size_t tsIdx = itTS.first;
        CampaignTimeSeries &ts = itTS.second;

        if (!ts.datasets.size())
            continue;

        auto &ds = m_CampaignData.datasets[ts.datasets.begin()->second];
        if (ds.format == FileFormat::IMAGE || ds.format == FileFormat::TEXT)
            continue;

        std::string localCachePath =
            m_Options.cachepath + PathSeparator + ds.uuid.substr(0, 3) + PathSeparator + ds.uuid;
        helper::CreateDirectory(localCachePath);
        std::string atsFilePath = localCachePath + PathSeparator + ts.name + ".ats";
        if (m_Options.verbose > 0)
        {
            std::cout << "    " << tsIdx << ". " << ts.name << "  --> " << atsFilePath << std::endl;
        }
        std::ofstream atsfile(atsFilePath);
        size_t atsLines = 0;

        adios2::core::IO &io = m_IO.m_ADIOS.DeclareIO("CampaignReader-TS-" + std::to_string(tsIdx));
        for (auto &itDS : ts.datasets)
        {
            size_t tsorder = itDS.first;
            size_t dsIdx = itDS.second;
            CampaignDataset &ds = m_CampaignData.datasets[dsIdx];
            if (!Matches(ds.name))
                continue;
            std::string localPath;
            size_t repIdx = m_CampaignData.FindReplicaOnHost(dsIdx, m_Options.hostname);
            if (!repIdx)
            {
                // pick the first replica. FIXME: need a smart way to pick a replica
                auto itRep = ds.replicas.begin();
                repIdx = itRep->first;
                std::string localPath = SaveRemoteMD(dsIdx, repIdx, io);
                if (!localPath.empty())
                {
                    if (m_Options.verbose > 0)
                    {
                        std::cout << "      " << tsorder << ". " << ds.name << " local file "
                                  << localPath << "\n";
                    }
                    CampaignDataset &ds = m_CampaignData.datasets[dsIdx];
                    CampaignReplica &rep = ds.replicas[repIdx];
                    atsfile << "- localpath: " << localPath << "\n  remotepath: "
                            << m_CampaignData.directory[rep.dirIdx].path + PathSeparator + rep.name
                            << "\n  remotehost: " << m_CampaignData.hosts[rep.hostIdx].hostname
                            << "\n  uuid: " << ds.uuid << std::endl;
                    ++atsLines;
                }
                else
                {
                    if (m_Options.verbose > 0)
                    {
                        std::cout << "      " << tsorder << ". " << ds.name << " Skipping \n";
                    }
                }
            }
            else
            {
                CampaignReplica &rep = ds.replicas[repIdx];
                localPath = m_CampaignData.directory[rep.dirIdx].path + PathSeparator + rep.name;
                if (m_Options.verbose > 0)
                {
                    std::cout << "      " << ds.tsorder << ". " << ds.name << " local file "
                              << localPath << "\n";
                }
                atsfile << "- " << localPath << std::endl;
                ++atsLines;
            }
        }
        atsfile << "- end" << std::endl;
        atsfile.close();
        if (atsLines > 0)
            OpenDatasetWithADIOS(ts.name, ds.format, io, atsFilePath);
    }

    // process individual datasets not in any time-series (and all images/texts)
    for (auto &it : m_CampaignData.datasets)
    {
        size_t dsIdx = it.first;
        CampaignDataset &ds = it.second;
        if (ds.tsid && (ds.format == FileFormat::HDF5 || ds.format == FileFormat::ADIOS))
            continue;
        if (ds.format == FileFormat::IMAGE)
            continue;
        if (!Matches(ds.name))
            continue;

        adios2::core::IO &io = m_IO.m_ADIOS.DeclareIO("CampaignReader" + std::to_string(dsIdx));
        std::string localPath;
        size_t repIdx = m_CampaignData.FindReplicaOnHost(dsIdx, m_Options.hostname);
        if (!repIdx)
        {
            /* FIXME: For now we only consider the first remote replica */
            size_t repIdx;
            auto itRep = ds.replicas.begin();
            repIdx = itRep->first;
            localPath = SaveRemoteMD(dsIdx, repIdx, io);
            if (!localPath.empty())
            {
                if (m_Options.verbose > 0)
                {
                    std::cout << "    " << ds.name << " local file " << localPath << "\n";
                }
            }
            else
            {
                if (m_Options.verbose > 0)
                {
                    std::cout << "    " << ds.name << " Skipping \n";
                }
            }
        }
        else
        {
            CampaignReplica &rep = ds.replicas[repIdx];
            localPath = m_CampaignData.directory[rep.dirIdx].path + PathSeparator + rep.name;
            if (m_Options.verbose > 0)
            {
                std::cout << "Open local file " << localPath << "\n";
            }
            if (ds.format == FileFormat::TEXT)
            {
                // TEXT -> create a variable
                CreateTextVariable(ds.name, adios2sys::SystemTools::FileLength(localPath), dsIdx,
                                   repIdx, localPath);
            }
        }

        if (ds.format == FileFormat::HDF5)
        {
            io.SetEngine("HDF5");
        }
        else if (ds.format == FileFormat::TEXT || ds.format == FileFormat::IMAGE)
        {
            continue;
        }
        else if (ds.format == FileFormat::Unknown)
        {
            continue;
        }
        OpenDatasetWithADIOS(ds.name, ds.format, io, localPath);
    }

    // process images separately as all resolutions are presented as different variables
    for (auto &it : m_CampaignData.datasets)
    {
        size_t dsIdx = it.first;
        CampaignDataset &ds = it.second;
        if (ds.format != FileFormat::IMAGE)
            continue;
        if (!Matches(ds.name))
            continue;
        std::string localPath;
        if (m_Options.verbose > 1)
        {
            std::cout << "-- Image " << ds.name << ":\n";
        }
        for (auto &itRep : ds.replicas)
        {
            size_t repIdx = itRep.first;
            CampaignReplica &rep = itRep.second;
            if (m_Options.verbose > 1)
            {
                std::cout << "      " << m_CampaignData.hosts[rep.hostIdx].hostname << ":"
                          << m_CampaignData.directory[rep.dirIdx].path << PathSeparator << rep.name
                          << "\n";
            }
            if (m_CampaignData.directory[rep.dirIdx].archive)
            {
                // this is an image in an archived place, skip
                continue;
            }
            else if (rep.files.size())
            {
                // this replica has embedded image
                // IMAGE -> create a variable, will read from DB directly, no local path
                std::string imgName =
                    ds.name + "/" + std::to_string(rep.x) + "x" + std::to_string(rep.y);
                if (m_Options.verbose > 1)
                {
                    std::cout << "        --- embedded image " << imgName << "\n";
                }
                auto f = rep.files.begin();
                CreateImageVariable(imgName, f->lengthOriginal, dsIdx, repIdx);
            }
            else if (m_CampaignData.hosts[rep.hostIdx].hostname == m_Options.hostname)
            {
                // this replica is local
                localPath = m_CampaignData.directory[rep.dirIdx].path + PathSeparator + rep.name;
                if (m_Options.verbose > 1)
                {
                    std::cout << "        --- local image " << localPath << " and resolution "
                              << rep.x << "x" << rep.y << std::endl;
                }
                std::string imgName =
                    ds.name + "/" + std::to_string(rep.x) + "x" + std::to_string(rep.y);
                CreateImageVariable(imgName, adios2sys::SystemTools::FileLength(localPath), dsIdx,
                                    repIdx, localPath);
            }
            else
            {
                // this is a remote image
                std::string imgName =
                    ds.name + "/" + std::to_string(rep.x) + "x" + std::to_string(rep.y);
                if (m_Options.verbose > 1)
                {
                    std::cout << "        --- remote image " << imgName << "\n";
                }
                CreateImageVariable(imgName, rep.size, dsIdx, repIdx);
            }
        }
    }
}

void CampaignReader::OpenDatasetWithADIOS(std::string prefixName, FileFormat format,
                                          adios2::core::IO &io, std::string &localPath)
{
    adios2::core::Engine &e = io.Open(localPath, m_OpenMode, m_Comm.Duplicate());

    m_IOs.push_back(&io);
    m_Engines.push_back(&e);

    auto vmap = io.GetAvailableVariables();
    auto amap = io.GetAvailableAttributes();
    VarInternalInfo internalInfo(nullptr, m_IOs.size() - 1, m_Engines.size() - 1);

    for (auto &vr : vmap)
    {
        auto vname = vr.first;
        std::string fname = prefixName;
        std::string newname;
        if (format == FileFormat::HDF5)
        {
            newname = fname + vname;
        }
        if (!vname.empty() && vname[0] == '/')
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
        std::string fname = prefixName;
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
    m_CampaignData.Close();
    m_IsOpen = false;
}

void CampaignReader::CreateDatasetAttributes(const std::string type, const std::string &name,
                                             const size_t dsIdx, const size_t repIdx,
                                             const std::string localPath)
{
    m_IO.DefineAttribute<std::string>("__dataset_type__", type, name);
    bool isLocal = false;
    if (!localPath.empty())
        isLocal = true; // file is on current host

    if (m_CampaignData.datasets[dsIdx].replicas[repIdx].files.size() > 0)
        isLocal = true; // file is embedded in archive

    // FIXME: in cache?

    m_IO.DefineAttribute<std::string>("__dataset_location__", (isLocal ? "local" : "remote"), name);
}

void CampaignReader::CreateTextVariable(const std::string &name, const size_t len,
                                        const size_t dsIdx, const size_t repIdx,
                                        const std::string localPath)
{
    // TEXT -> create a variable, will read from DB directly, no local path
    Variable<char> &v = m_IO.DefineVariable<char>(name, Dims{len}, Dims{0ULL}, Dims{len}, false);
    v.m_AvailableStepsCount = 1;
    v.m_AvailableStepsStart = 0;
    CampaignVarInternalInfo internalInfo(&v, dsIdx, repIdx, localPath);
    m_CampaignVarInternalInfo.emplace(v.m_Name, internalInfo);
    CreateDatasetAttributes("text", name, dsIdx, repIdx, localPath);
}

void CampaignReader::CreateImageVariable(const std::string &name, const size_t len,
                                         const size_t dsIdx, const size_t repIdx,
                                         const std::string localPath)
{
    // TEXT -> create a variable, will read from DB directly, no local path
    Variable<uint8_t> &v =
        m_IO.DefineVariable<uint8_t>(name, Dims{len}, Dims{0ULL}, Dims{len}, false);
    v.m_AvailableStepsCount = 1;
    v.m_AvailableStepsStart = 0;
    CampaignVarInternalInfo internalInfo(&v, dsIdx, repIdx, localPath);
    m_CampaignVarInternalInfo.emplace(v.m_Name, internalInfo);
    CreateDatasetAttributes("image", name, dsIdx, repIdx, localPath);
}

void CampaignReader::DestructorClose(bool Verbose) noexcept { m_CampaignData.Close(); }

bool CampaignReader::Matches(const std::string &dsname)
{
    for (const auto &re : m_ExcludePatternsRe)
    {
        if (std::regex_match(dsname, re))
        {
            return false;
        }
    }

    if (!m_IncludePatternsRe.empty())
    {
        for (const auto &re : m_IncludePatternsRe)
        {
            if (std::regex_match(dsname, re))
            {
                return true;
            }
        }
        // If no match in include patterns and the list is not empty, return false
        return false;
    }

    // If no match in exclude list while include list is empty, return true
    return true;
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
            switch (m_CampaignData.datasets.at(it->second.dsIdx).format)
            {
            case FileFormat::TEXT:
                Blk.MinMax.Init(adios2::DataType::Char);
                Blk.MinMax.MinUnion.field_int8 = (int8_t)'a';
                Blk.MinMax.MaxUnion.field_int8 = (int8_t)'z';
                MV->BlocksInfo.push_back(Blk);
                break;
            case FileFormat::IMAGE:
                Blk.MinMax.Init(adios2::DataType::UInt8);
                Blk.MinMax.MinUnion.field_uint8 = 0;
                Blk.MinMax.MaxUnion.field_uint8 = 255;
                MV->BlocksInfo.push_back(Blk);
                break;
            default:
                break;
            }
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
            switch (m_CampaignData.datasets[it->second.dsIdx].format)
            {
            case FileFormat::TEXT:
                MinMax.Init(adios2::DataType::Char);
                MinMax.MinUnion.field_int8 = (int8_t)'A';
                MinMax.MaxUnion.field_int8 = (int8_t)'Z';
                return true;
            case FileFormat::IMAGE:
                MinMax.Init(adios2::DataType::UInt8);
                MinMax.MinUnion.field_uint8 = 1;
                MinMax.MaxUnion.field_uint8 = 254;
                return true;
            default:
                return false;
            }
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

void CampaignReader::GetVariableFromDB(std::string name, size_t dsIdx, size_t repIdx, DataType type,
                                       void *data)
{
    if (type != DataType::Char && type != DataType::UInt8)
    {
        helper::Throw<std::invalid_argument>(
            "Engine", "CampaignReader", "GetVariableFromDB",
            "Expected char/uint8_t variable but was called with a type " + ToString(type));
    }

    CampaignReplica &rep = m_CampaignData.datasets[dsIdx].replicas[repIdx];
    const CampaignFile &f = rep.files.at(0);
    std::string keyhex;
    if (rep.hasKey)
        keyhex = m_CampaignData.keys[rep.keyIdx].keyHex;
    m_CampaignData.ReadToMemory((char *)data, f, keyhex);
}

void CampaignReader::ReadRemoteFile(const std::string &remoteHost, const std::string &remotePath,
                                    const size_t size, void *data)
{
    std::unique_ptr<Remote> remote = nullptr;
#ifdef ADIOS2_HAVE_XROOTD
    if (getenv("DoXRootD"))
    {
        remote = std::unique_ptr<XrootdRemote>(new XrootdRemote(m_HostOptions));
        remote->Open("localhost", 1094, m_Name, m_OpenMode, true);
    }
    else
#endif
#ifdef ADIOS2_HAVE_SST
    {
        remote = std::unique_ptr<EVPathRemote>(new EVPathRemote(m_HostOptions));
        int localPort = remote->LaunchRemoteServerViaConnectionManager(remoteHost);
        remote->OpenSimpleFile("localhost", localPort, remotePath);
    }
#endif
    // evaluate validity of object, not just that the pointer is non-NULL
    if (remote == nullptr || !(*remote))
    {
        helper::Throw<std::ios_base::failure>(
            "Engine", "CampaignReader", "ReadRemoteFile",
            "Cannot get remote connection to read file " + remoteHost + ":" + remotePath +
                ". Make sure connection manager is running, and the server specification for the "
                "host is valid.");
    }
    remote->Read(0, size, data);
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

} // end namespace engine
} // end namespace core
} // end namespace adios2
