/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CampaignIndexReader.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/toolkit/remote/Remote.h"
#include "adios2/toolkit/transport/OpenFile.h"
#include <filesystem>
#include <fstream>

#include <nlohmann_json.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <numeric>
#include <sstream>
#include <type_traits>

namespace adios2
{
namespace core
{
namespace engine
{
namespace
{
template <class T>
T Number(const std::string &text)
{
    if constexpr (std::is_same<T, std::string>::value)
        return text;
    else if constexpr (std::is_integral<T>::value && std::is_signed<T>::value)
        return static_cast<T>(std::stoll(text));
    else if constexpr (std::is_integral<T>::value)
        return static_cast<T>(std::stoull(text));
    else if constexpr (std::is_floating_point<T>::value)
    {
        // strtold also accepts the nan/inf spellings in ADIOS statistics.
        char *end = nullptr;
        const long double value = std::strtold(text.c_str(), &end);
        if (end == text.c_str() || *end != '\0')
            throw std::runtime_error("CampaignIndex: invalid numeric metadata: " + text);
        return static_cast<T>(value);
    }
    else
    {
        T value;
        std::istringstream input(text);
        if (!(input >> value))
            throw std::runtime_error("CampaignIndex: invalid complex metadata: " + text);
        return value;
    }
}

template <class T>
T JSONValue(const nlohmann::json &value)
{
    if constexpr (std::is_arithmetic<T>::value || std::is_same<T, std::string>::value)
        return value.get<T>();
    else
    {
        if (value.is_string())
            return Number<T>(value.get<std::string>());
        return T(value.at(0).get<typename T::value_type>(),
                 value.at(1).get<typename T::value_type>());
    }
}

template <class T>
void DefineEntry(IO &io, Engine *engine, const std::string &name, const Dims &shape, size_t steps,
                 const std::string &minimum, const std::string &maximum, const std::string &value)
{
    auto &v = io.DefineVariable<T>(name, shape, Dims(shape.size(), 0), shape);
    v.m_Engine = engine;
    v.m_RandomAccess = engine->OpenMode() == Mode::ReadRandomAccess;
    v.m_AvailableStepsCount = steps;
    v.m_AvailableStepsStart = 0;
    // The index has one summary shape, rather than per-step shapes.
    if (!minimum.empty() && !maximum.empty())
    {
        v.m_Min = Number<T>(minimum);
        v.m_Max = Number<T>(maximum);
    }
    else if constexpr (std::is_arithmetic<T>::value)
    {
        MinMaxStruct unset;
        unset.Init(v.m_Type);
        std::memcpy(&v.m_Min, &unset.MinUnion, sizeof(T));
        std::memcpy(&v.m_Max, &unset.MaxUnion, sizeof(T));
    }
    if (v.m_SingleValue && steps == 1 && !value.empty())
    {
        auto j = nlohmann::json::parse(value);
        if (j.is_array() && j.size() == 1)
            j = nlohmann::json(j.at(0));
        if (!j.is_null())
            v.m_Value = JSONValue<T>(j);
    }
}

template <class T>
void DefineIndexAttribute(IO &io, const std::string &name, const std::string &text)
{
    const auto j = nlohmann::json::parse(text);
    if (j.is_array())
    {
        std::vector<T> values;
        values.reserve(j.size());
        for (const auto &item : j)
            values.push_back(JSONValue<T>(item));
        io.DefineAttribute<T>(name, values.data(), values.size());
    }
    else
        io.DefineAttribute<T>(name, JSONValue<T>(j));
}
} // namespace

CampaignIndexReader::CampaignIndexReader(IO &io, const std::string &name, Mode mode,
                                         helper::Comm comm)
: Engine("CampaignIndexReader", io, name, mode, std::move(comm))
{
    if (mode != Mode::Read && mode != Mode::ReadRandomAccess)
        throw std::invalid_argument("CampaignIndex only supports reading");
    Init();
    m_IsOpen = true;
}

void CampaignIndexReader::Init()
{
    m_IO.m_ReadStreaming = false;
    const auto index = CampaignData::ReadIndex(m_Name);
    for (const auto &variable : index.variables)
    {
        const auto &name = variable.name;
        m_Entries.emplace(name, variable.source);
        bool supported = false;
#define declare_type(T)                                                                            \
    if (variable.type == helper::GetDataType<T>())                                                 \
    {                                                                                              \
        DefineEntry<T>(m_IO, this, name, variable.shape, variable.steps, variable.minimum,         \
                       variable.maximum, variable.value);                                          \
        supported = true;                                                                          \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        if (!supported)
            throw std::runtime_error("CampaignIndex: unsupported type for " + name);
        m_Steps = std::max(m_Steps, variable.steps);
    }
    for (const auto &attribute : index.attributes)
    {
        bool supported = false;
#define declare_type(T)                                                                            \
    if (attribute.type == helper::GetDataType<T>())                                                \
    {                                                                                              \
        DefineIndexAttribute<T>(m_IO, attribute.name, attribute.value);                            \
        supported = true;                                                                          \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        if (!supported)
            throw std::runtime_error("CampaignIndex: unsupported attribute type for " +
                                     attribute.name);
    }
}

StepStatus CampaignIndexReader::BeginStep(StepMode mode, float)
{
    if (m_OpenMode == Mode::ReadRandomAccess || mode != StepMode::Read || m_InStep)
        throw std::logic_error("CampaignIndex: invalid BeginStep call");
    if (m_CurrentStep >= m_Steps)
        return StepStatus::EndOfStream;
    m_IO.m_ReadStreaming = true;
    m_IO.m_EngineStep = m_CurrentStep;
    for (const auto &entry : m_IO.GetVariables())
    {
        entry.second->ResetStepsSelection(false);
        entry.second->m_StepPresent =
            m_CurrentStep < entry.second->m_AvailableStepsCount ? m_CurrentStep : MaxSizeT - 1;
    }
    m_InStep = true;
    return StepStatus::OK;
}
size_t CampaignIndexReader::CurrentStep() const { return m_CurrentStep; }
void CampaignIndexReader::EndStep()
{
    if (!m_InStep)
        throw std::logic_error("CampaignIndex: EndStep without BeginStep");
    PerformGets();
    m_InStep = false;
    ++m_CurrentStep;
}
void CampaignIndexReader::PerformGets()
{
    // Remove a request only after it succeeds, so a failed read remains visible.
    while (!m_PendingGets.empty())
    {
        m_PendingGets.front()();
        m_PendingGets.pop_front();
    }
}
void CampaignIndexReader::DoClose(int)
{
    PerformGets();
    m_Archives.clear();
    m_InStep = false;
    m_IO.m_ReadStreaming = false;
}
size_t CampaignIndexReader::DoSteps() const { return m_Steps; }
void CampaignIndexReader::DoGetAbsoluteSteps(const VariableBase &v,
                                             std::vector<size_t> &steps) const
{
    steps.resize(v.m_AvailableStepsCount);
    std::iota(steps.begin(), steps.end(), 0);
}
bool CampaignIndexReader::VarShape(const VariableBase &v, size_t step, Dims &shape) const
{
    if (step != DefaultSizeT && step >= v.m_AvailableStepsCount)
        return false;
    shape = v.m_Shape;
    return true;
}
MinVarInfo *CampaignIndexReader::MinBlocksInfo(const VariableBase &v, size_t step) const
{
    if (step >= v.m_AvailableStepsCount)
        return nullptr;
    // Expose the summary shape, but no invented block decomposition.
    auto *info = new MinVarInfo(static_cast<int>(v.m_Shape.size()), v.m_Shape.data());
    info->Step = step;
    info->WasLocalValue = false;
    info->IsValue = v.m_SingleValue;
    return info;
}
bool CampaignIndexReader::VariableMinMax(const VariableBase &v, size_t step, MinMaxStruct &mm)
{
    mm.Init(v.m_Type);
    // Multi-step indexes contain aggregate extrema only.
    if (step != DefaultSizeT && (step >= v.m_AvailableStepsCount || v.m_AvailableStepsCount > 1))
        return true;
#define declare_type(T)                                                                            \
    if constexpr (std::is_arithmetic<T>::value)                                                    \
    {                                                                                              \
        if (v.m_Type == helper::GetDataType<T>())                                                  \
        {                                                                                          \
            const auto &variable = static_cast<const Variable<T> &>(v);                            \
            std::memcpy(&mm.MinUnion, &variable.m_Min, sizeof(T));                                 \
            std::memcpy(&mm.MaxUnion, &variable.m_Max, sizeof(T));                                 \
        }                                                                                          \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    return true;
}

CampaignIndexReader::Archive &CampaignIndexReader::OpenArchive(const std::string &name)
{
    auto it = m_Archives.find(name);
    if (it != m_Archives.end())
        return *it->second;

    std::string store = m_UserOptions.campaign.campaignstorepath;
    for (const auto &p : m_IO.m_Parameters)
        if (helper::LowerCase(p.first) == "campaignstorepath")
            store = p.second;
    const std::filesystem::path archive(name);
    std::vector<std::filesystem::path> paths;
    if (archive.is_absolute())
        paths.push_back(archive);
    else
    {
        if (!store.empty())
            paths.push_back(std::filesystem::path(store) / archive);
        // Indexes can be moved together with their sibling archives. An index
        // within a store may name archives relative to that store's parent.
        auto parent = std::filesystem::path(m_Name).parent_path();
        paths.push_back(parent / archive);
        paths.push_back(parent.parent_path() / archive);
        paths.push_back(parent / archive.filename());
        paths.push_back(archive);
    }
    for (const auto &path : paths)
    {
        if (!std::filesystem::exists(path))
            continue;
        auto db = std::make_unique<Archive>();
        db->Open(path.string());
        db->ReadDatabase();
        auto &result = *db;
        m_Archives.emplace(name, std::move(db));
        return result;
    }
    throw std::runtime_error("CampaignIndex: cannot locate archive " + name);
}

void CampaignIndexReader::ReadBytes(const Entry &entry, size_t start, size_t count, char *data)
{
    auto &archive = OpenArchive(entry.archive);
    auto dsIt = archive.datasets.find(entry.dataset);
    if (dsIt == archive.datasets.end() || dsIt->second.name != entry.datasetName)
        throw std::runtime_error("CampaignIndex: dataset no longer matches archive: " +
                                 entry.datasetName);
    auto &ds = dsIt->second;
    const bool image = entry.format == "IMAGE";
    if ((image && ds.format != FileFormat::IMAGE) || (!image && ds.format != FileFormat::TEXT))
        throw std::runtime_error("CampaignIndex: dataset format no longer matches index");
    auto matches = [&](const CampaignReplica &rep) {
        return !rep.deleted &&
               (!image || entry.name == std::to_string(rep.x) + "x" + std::to_string(rep.y));
    };
    auto check = [&](size_t length) {
        if (start > length || count > length - start)
            throw std::invalid_argument("CampaignIndex: byte selection exceeds replica size");
    };
    // Embedded data takes priority even when a local or remote replica exists.
    for (auto &item : ds.replicas)
    {
        auto &rep = item.second;
        if (!matches(rep) || rep.files.empty())
            continue;
        const auto file = rep.files.front();
        const auto length = archive.files.at(file).lengthOriginal;
        check(length);
        std::string key;
        if (rep.hasKey)
        {
            auto &k = archive.keys.at(rep.keyIdx);
            if (k.keyHex.empty())
            {
                Remote connection;
                k.keyHex = connection.GetKeyFromConnectionManager(k.id);
            }
            if (k.keyHex == "0")
                throw std::runtime_error("CampaignIndex: no key for embedded file");
            key = k.keyHex;
        }
        // Decompression/decryption require the whole embedded object. Copy only
        // the requested bytes into the caller's (possibly smaller) buffer.
        std::vector<char> bytes(length);
        archive.ReadToMemory(bytes.data(), file, key);
        if (count)
            std::memcpy(data, bytes.data() + start, count);
        return;
    }

    std::string hostname = m_UserOptions.campaign.hostname;
    for (const auto &p : m_IO.m_Parameters)
        if (helper::LowerCase(p.first) == "hostname")
            hostname = p.second;
    if (hostname.empty())
        hostname = helper::GetClusterName();
    std::vector<std::string> localHosts{hostname};
    for (const auto &host : ADIOS::GetHostOptions())
        for (const auto &config : host.second)
            if (config.protocol == HostAccessProtocol::LocalHost)
                localHosts.push_back(host.first);
    auto replicas = archive.FindRemoteReplicas(entry.dataset, ADIOS::GetHostOptions());
    for (const auto &item : ds.replicas)
        if (std::find(localHosts.begin(), localHosts.end(),
                      archive.hosts.at(item.second.hostIdx).hostname) != localHosts.end())
            replicas.insert(replicas.begin(), item.first);

    for (const auto id : replicas)
    {
        auto &rep = ds.replicas.at(id);
        if (!matches(rep))
            continue;
        const auto &host = archive.hosts.at(rep.hostIdx);
        const bool local =
            std::find(localHosts.begin(), localHosts.end(), host.hostname) != localHosts.end();
        std::string filename = rep.name;
        size_t offset = 0, length = rep.size;
        auto tar = archive.tarnames.find(rep.archiveIdx);
        if (rep.archiveIdx && tar != archive.tarnames.end())
        {
            filename = tar->second;
            const auto info =
                helper::StringToTarInfo(archive.GetTarIdx(entry.dataset, id, ds.format));
            if (info.size() != 1)
                throw std::runtime_error("CampaignIndex: missing or ambiguous TAR byte offsets");
            offset = std::get<0>(info.begin()->second);
            length = std::get<1>(info.begin()->second);
        }
        std::string path = archive.directory.at(rep.dirIdx).path + PathSeparator + filename;
        if (local)
        {
            std::ifstream input(path, std::ios::binary);
            if (!input)
                continue;
            if (!rep.archiveIdx)
            {
                input.seekg(0, std::ios::end);
                length = static_cast<size_t>(input.tellg());
            }
            check(length);
            input.seekg(offset + start);
            input.read(data, count);
            if (!input)
                throw std::runtime_error("CampaignIndex: incomplete read of " + path);
            return;
        }
        check(length);
        if (host.defaultProtocol == "https" || host.defaultProtocol == "s3")
        {
            const HostConfig *config = nullptr;
            auto options = ADIOS::GetHostOptions().find(host.hostname);
            if (options != ADIOS::GetHostOptions().end())
                for (const auto &c : options->second)
                    if ((host.defaultProtocol == "https" &&
                         c.protocol == HostAccessProtocol::HTTPS) ||
                        (host.defaultProtocol == "s3" && c.protocol == HostAccessProtocol::S3))
                    {
                        config = &c;
                        break;
                    }
            Params params{{"cache", m_UserOptions.campaign.cachepath},
                          {"recheck_metadata", "false"}};
            if (host.defaultProtocol == "https")
            {
                std::string endpoint = config ? config->endpoint : "https://" + host.longhostname;
                if (!endpoint.empty() && endpoint.back() != '/')
                    endpoint += '/';
                path = endpoint + (path.front() == '/' ? path.substr(1) : path);
                params["library"] = "https";
                if (config && !config->caFile.empty())
                    params["ca_file"] = config->caFile;
            }
            else
            {
                if (!config)
                    throw std::runtime_error("CampaignIndex: no S3 host configuration for " +
                                             host.hostname);
                params["library"] = "awssdk";
                params["endpoint"] = config->endpoint;
                helper::PutEnv(std::string("AWS_PROFILE=") +
                               (config->awsProfile.empty() ? "default" : config->awsProfile));
                helper::PutEnv(std::string("AWS_EC2_METADATA_DISABLED=") +
                               (config->isAWS_EC2 ? "false" : "true"));
            }
            helper::Comm comm;
            auto file = transport::OpenFile(comm, path, Mode::Read, params, false);
            file->Read(data, count, offset + start);
            file->Close();
        }
        else
        {
            const auto setup = GetRemoteSetup(host.hostname);
            if (setup.protocol != HostAccessProtocol::SSH &&
                setup.protocol != HostAccessProtocol::XRootD)
                throw std::runtime_error("CampaignIndex: no SSH/XRootD configuration for " +
                                         host.hostname);
            auto remote = GetRemoteSimpleFile(setup, path);
            if (!remote || !*remote)
                throw std::runtime_error("CampaignIndex: cannot open remote file " + path);
            try
            {
                auto handle = remote->Read(offset + start, count, data);
                if (setup.protocol == HostAccessProtocol::XRootD)
                {
                    if (!handle || !remote->WaitForGet(handle))
                        throw std::runtime_error("CampaignIndex: incomplete remote read of " +
                                                 path);
                }
            }
            catch (...)
            {
                remote->Close();
                throw;
            }
            remote->Close();
        }
        return;
    }
    throw std::runtime_error("CampaignIndex: no readable replica for " + entry.datasetName);
}

template <class T>
void CampaignIndexReader::GetSync(Variable<T> &v, T *data)
{
    // Parsed text fields and other scalar summaries are cached in the index.
    // Serve them before dispatching raw image/text bytes to the archive.
    if (v.m_SingleValue && v.m_AvailableStepsCount == 1)
    {
        *data = v.m_Value;
        return;
    }
    const auto &entry = m_Entries.at(v.m_Name);
    if (entry.format == "IMAGE" || entry.format == "TEXT")
    {
        if (v.m_Type != DataType::Char && v.m_Type != DataType::UInt8)
            throw std::runtime_error("CampaignIndex: expected byte variable for image/text");
        if (v.m_Shape.size() != 1 || v.m_Start.size() != 1 || v.m_Count.size() != 1 ||
            !v.m_MemoryCount.empty() ||
            (v.m_SelectionType != SelectionType::BoundingBox &&
             v.m_SelectionType != SelectionType::All) ||
            v.m_StepsStart != 0 || v.m_StepsCount != 1)
            throw std::invalid_argument(
                "CampaignIndex: image/text requires a one-dimensional byte selection at step zero");
        if (v.m_Start[0] > v.m_Shape[0] || v.m_Count[0] > v.m_Shape[0] - v.m_Start[0])
            throw std::invalid_argument("CampaignIndex: byte selection exceeds indexed shape");
        if (v.GetMemorySpace(data) != MemorySpace::Host)
            throw std::invalid_argument("CampaignIndex: image/text requires host memory");
        ReadBytes(entry, v.m_Start[0], v.m_Count[0], reinterpret_cast<char *>(data));
        return;
    }
    throw std::runtime_error("CampaignIndex: data reads for " + entry.format +
                             " are not implemented");
}

#define declare_type(T)                                                                            \
    void CampaignIndexReader::DoGetSync(Variable<T> &v, T *data) { GetSync(v, data); }             \
    void CampaignIndexReader::DoGetDeferred(Variable<T> &v, T *data)                               \
    {                                                                                              \
        m_PendingGets.emplace_back([this, v, data]() mutable { GetSync(v, data); });               \
    }                                                                                              \
    std::vector<typename Variable<T>::BPInfo> CampaignIndexReader::DoBlocksInfo(                   \
        const Variable<T> &, size_t) const                                                         \
    {                                                                                              \
        return {};                                                                                 \
    }                                                                                              \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>>                                    \
    CampaignIndexReader::DoAllStepsBlocksInfo(const Variable<T> &) const                           \
    {                                                                                              \
        return {};                                                                                 \
    }                                                                                              \
    std::vector<std::vector<typename Variable<T>::BPInfo>>                                         \
    CampaignIndexReader::DoAllRelativeStepsBlocksInfo(const Variable<T> &) const                   \
    {                                                                                              \
        return {};                                                                                 \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

} // namespace engine
} // namespace core
} // namespace adios2
