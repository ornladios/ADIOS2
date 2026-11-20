/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CampaignIndexReader.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/DefinitionBatch.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/toolkit/remote/Remote.h"
#include "adios2/toolkit/transport/OpenFile.h"
#include <filesystem>
#include <fstream>

#include <nlohmann_json.hpp>

#include <algorithm>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <numeric>
#include <sstream>
#include <thread>
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
void DefineEntry(DefinitionBatch &io, Engine *engine, const std::string &name, const Dims &shape,
                 size_t steps, const std::string &minimum, const std::string &maximum,
                 const std::string &value)
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
void DefineIndexAttribute(DefinitionBatch &io, const std::string &name, const std::string &text)
{
    if (io.InquireAttribute<T>(name))
        throw std::invalid_argument("CampaignIndex: duplicate attribute: " + name);
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

// A slot is reused only after its previous definitions have been merged. The
// caller alone streams SQLite and mutates the destination IO and source map.
// Fixed worker threads own private batches; round-robin draining preserves row
// order, bounds temporary storage, and applies backpressure to the streamer.
class IndexDefinitionLoader
{
public:
    using Entries = std::unordered_map<std::string, CampaignIndexEntry>;
    IndexDefinitionLoader(IO &io, Engine *engine, Entries &entries, size_t &steps, size_t threads,
                          size_t batchSize)
    : m_IO(io), m_Engine(engine), m_Entries(entries), m_Steps(steps), m_Parallel(threads > 1)
    {
        for (size_t i = 0; i < threads; ++i)
        {
            auto worker = std::make_unique<Worker>();
            worker->definitions = io.CreateDefinitionBatch(batchSize);
            m_Workers.push_back(std::move(worker));
        }
        try
        {
            if (m_Parallel)
                for (auto &worker : m_Workers)
                    worker->thread = std::thread([this, w = worker.get()] { Run(*w); });
        }
        catch (...)
        {
            Stop();
            throw;
        }
    }
    ~IndexDefinitionLoader() { Stop(); }

    void Submit(CampaignIndex &rows)
    {
        auto &worker = *m_Workers[m_Next];
        Merge(worker);
        std::swap(rows, worker.rows);
        if (m_Parallel)
        {
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                worker.state = State::Working;
            }
            m_Condition.notify_all();
        }
        else
        {
            Build(worker);
            worker.state = State::Done;
            Merge(worker);
        }
        m_Next = (m_Next + 1) % m_Workers.size();
    }

    void Finish()
    {
        for (size_t i = 0; i < m_Workers.size(); ++i)
            Merge(*m_Workers[(m_Next + i) % m_Workers.size()]);
        m_IO.FinalizeDefinitions();
    }

private:
    enum class State
    {
        Idle,
        Working,
        Done
    };
    struct Worker
    {
        CampaignIndex rows;
        std::unique_ptr<DefinitionBatch> definitions;
        Entries entries;
        size_t steps = 0;
        State state = State::Idle;
        std::exception_ptr error;
        std::thread thread;
    };

    void Build(Worker &worker)
    {
        for (auto &variable : worker.rows.variables)
        {
            // Parsed TEXT fields can contain whole JSON objects or arrays.
            // ADIOS strings are scalar: expose the serialized JSON as one string.
            if (variable.type == DataType::String && variable.source.format == "TEXT" &&
                !variable.value.empty())
            {
                const auto value = nlohmann::json::parse(variable.value);
                if (value.is_structured())
                {
                    variable.dimensions = 0;
                    variable.shape.clear();
                    variable.value = nlohmann::json(variable.value).dump();
                }
            }
            Dims shape;
            std::istringstream dimensions(variable.shape);
            size_t extent;
            while (dimensions >> extent)
            {
                shape.push_back(extent);
                dimensions >> std::ws;
                if (dimensions.peek() == ',')
                    dimensions.get();
            }
            if (variable.dimensions != static_cast<int64_t>(shape.size()))
                throw std::runtime_error("CampaignIndex: unsupported shape for " + variable.name);
            bool supported = false;
#define declare_type(T)                                                                            \
    if (variable.type == helper::GetDataType<T>())                                                 \
    {                                                                                              \
        DefineEntry<T>(*worker.definitions, m_Engine, variable.name, shape, variable.steps,        \
                       variable.minimum, variable.maximum, variable.value);                        \
        supported = true;                                                                          \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
            if (!supported)
                throw std::runtime_error("CampaignIndex: unsupported type for " + variable.name);
            worker.steps = std::max(worker.steps, variable.steps);
            worker.entries.emplace(std::move(variable.name), std::move(variable.source));
        }
        for (const auto &attribute : worker.rows.attributes)
        {
            bool supported = false;
#define declare_type(T)                                                                            \
    if (attribute.type == helper::GetDataType<T>())                                                \
    {                                                                                              \
        DefineIndexAttribute<T>(*worker.definitions, attribute.name, attribute.value);             \
        supported = true;                                                                          \
    }
            ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
            if (!supported)
                throw std::runtime_error("CampaignIndex: unsupported attribute type for " +
                                         attribute.name);
        }
        worker.rows.variables.clear();
        worker.rows.attributes.clear();
    }

    void Run(Worker &worker)
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        for (;;)
        {
            m_Condition.wait(lock, [&] { return m_Stop || worker.state == State::Working; });
            if (m_Stop)
                return;
            lock.unlock();
            try
            {
                Build(worker);
            }
            catch (...)
            {
                worker.error = std::current_exception();
            }
            lock.lock();
            worker.state = State::Done;
            m_Condition.notify_all();
        }
    }

    void Merge(Worker &worker)
    {
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_Condition.wait(lock, [&] { return worker.state != State::Working; });
            if (worker.state == State::Idle)
                return;
        }
        if (worker.error)
            std::rethrow_exception(worker.error);
        m_IO.MergeDefinitions(*worker.definitions);
        m_Entries.merge(worker.entries);
        m_Steps = std::max(m_Steps, worker.steps);
        worker.steps = 0;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            worker.state = State::Idle;
        }
    }

    void Stop()
    {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Stop = true;
        }
        m_Condition.notify_all();
        for (auto &worker : m_Workers)
            if (worker->thread.joinable())
                worker->thread.join();
    }

    IO &m_IO;
    Engine *m_Engine;
    Entries &m_Entries;
    size_t &m_Steps;
    const bool m_Parallel;
    std::vector<std::unique_ptr<Worker>> m_Workers;
    size_t m_Next = 0;
    std::mutex m_Mutex;
    std::condition_variable m_Condition;
    bool m_Stop = false;
};

size_t PositiveParameter(const std::string &name, const std::string &value)
{
    if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("CampaignIndex: " + name + " must be a positive integer");
    size_t used = 0;
    const auto result = std::stoull(value, &used);
    if (!result || result > std::numeric_limits<size_t>::max() || used != value.size())
        throw std::invalid_argument("CampaignIndex: " + name + " must be a positive integer");
    return static_cast<size_t>(result);
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

CampaignIndexReader::~CampaignIndexReader()
{
    // Pending buffers may no longer exist when an unclosed reader is destroyed.
    m_PendingGets.clear();
    try
    {
        Close();
    }
    catch (...)
    {
    }
}

void CampaignIndexReader::Init()
{
    m_IO.m_ReadStreaming = false;
    size_t threads = std::min(4u, std::max(1u, std::thread::hardware_concurrency()));
    size_t batchSize = 4096;
    for (const auto &parameter : m_IO.m_Parameters)
    {
        const auto key = helper::LowerCase(parameter.first);
        if (key == "metadatathreads")
            threads = PositiveParameter(parameter.first, parameter.second);
        else if (key == "metadatabatchsize")
            batchSize = PositiveParameter(parameter.first, parameter.second);
    }
    IndexDefinitionLoader loader(m_IO, this, m_Entries, m_Steps, threads, batchSize);
    CampaignData::StreamIndex(m_Name, batchSize, [&](CampaignIndex &rows) { loader.Submit(rows); });
    loader.Finish();
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
    for (auto &item : m_DatasetEngines)
    {
        item.second.engine->Close();
    }
    m_BlockStorage.clear();
    m_DatasetEngines.clear();
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
    if (m_InStep)
        step = m_CurrentStep;
    if (step >= v.m_AvailableStepsCount)
        return nullptr;
    const auto &entry = m_Entries.at(v.m_Name);
    if (entry.format == "ADIOS" || entry.format == "HDF5")
    {
        const auto source = FindSource(v, step);
        auto *info = source.engine->MinBlocksInfo(*source.variable, source.step);
        if (info)
        {
            info->Step = step; // The index exposes contiguous logical steps.
            return info;
        }
#define declare_type(T)                                                                            \
    if (v.m_Type == helper::GetDataType<T>())                                                      \
        return FallbackMinBlocksInfo(static_cast<const Variable<T> &>(v), step);
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        throw std::runtime_error("CampaignIndex: unsupported block metadata type");
    }
    // Byte/text entries have no source engine or physical block decomposition.
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

CampaignIndexReader::Archive &CampaignIndexReader::OpenArchive(const std::string &name) const
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

std::vector<size_t> CampaignIndexReader::ResolveDatasets(Archive &archive, const Entry &entry) const
{
    const auto dataset = archive.datasets.find(entry.dataset);
    if (dataset != archive.datasets.end() && dataset->second.name == entry.datasetName)
        return {dataset->first};
    // A series is named by its logical dataset name; its ID need not equal a
    // member dataset's rowid. Resolve membership from the ACA, in tsorder order.
    for (const auto &item : archive.timeseries)
    {
        if (item.second.name != entry.datasetName)
            continue;
        std::vector<size_t> result;
        for (const auto &member : item.second.datasets)
            result.push_back(member.second);
        if (!result.empty())
            return result;
    }
    throw std::runtime_error("CampaignIndex: dataset no longer matches archive: " +
                             entry.datasetName);
}

CampaignIndexReader::DatasetEngine &CampaignIndexReader::OpenDataset(const Entry &entry,
                                                                     size_t dataset) const
{
    const std::string key = entry.archive + "/" + std::to_string(dataset);
    auto cached = m_DatasetEngines.find(key);
    if (cached != m_DatasetEngines.end())
        return cached->second;
    auto &archive = OpenArchive(entry.archive);
    auto &ds = archive.datasets.at(dataset);
    if (ds.format != FileFormat(entry.format))
        throw std::runtime_error("CampaignIndex: dataset format no longer matches index");

    std::string hostname = m_UserOptions.campaign.hostname;
    std::string cache = m_UserOptions.campaign.cachepath;
    for (const auto &p : m_IO.m_Parameters)
    {
        if (helper::LowerCase(p.first) == "hostname")
            hostname = p.second;
        if (helper::LowerCase(p.first) == "cachepath")
            cache = p.second;
    }
    if (hostname.empty())
        hostname = helper::GetClusterName();
    std::vector<std::string> localHosts{hostname};
    for (const auto &host : ADIOS::GetHostOptions())
        for (const auto &config : host.second)
            if (config.protocol == HostAccessProtocol::LocalHost)
                localHosts.push_back(host.first);
    auto replicas = archive.FindRemoteReplicas(dataset, ADIOS::GetHostOptions());
    for (const auto &item : ds.replicas)
        if (!item.second.deleted &&
            std::find(localHosts.begin(), localHosts.end(),
                      archive.hosts.at(item.second.hostIdx).hostname) != localHosts.end())
            replicas.insert(replicas.begin(), item.first);

    for (const auto id : replicas)
    {
        auto &rep = ds.replicas.at(id);
        if (rep.deleted)
            continue;
        const auto &host = archive.hosts.at(rep.hostIdx);
        const bool local =
            std::find(localHosts.begin(), localHosts.end(), host.hostname) != localHosts.end();
        std::string path = archive.directory.at(rep.dirIdx).path + PathSeparator + rep.name;
        std::string tar;
        if (rep.archiveIdx)
        {
            tar = archive.GetTarIdx(dataset, id, ds.format);
            if (tar.empty())
                continue;
            path = archive.directory.at(rep.dirIdx).path + PathSeparator +
                   archive.tarnames.at(rep.archiveIdx);
        }
        if (local && !std::filesystem::exists(path))
            continue;
        const std::string ioName = "CampaignIndex-" +
                                   std::to_string(reinterpret_cast<uintptr_t>(this)) + "-" +
                                   std::to_string(m_DatasetEngines.size());
        // Own the inner IO so its lifetime does not depend on the order in
        // which the parent ADIOS destroys its named IOs.
        auto ownedIO = std::make_unique<IO>(m_IO.m_ADIOS, ioName, false, m_IO.m_HostLanguage);
        auto &io = *ownedIO;
        io.m_ArrayOrder = m_IO.m_ArrayOrder;
        if (ds.format == FileFormat::HDF5)
            io.SetEngine("HDF5");
        if (!tar.empty())
        {
            io.SetParameter("TarInfo", tar);
            if (ds.format == FileFormat::ADIOS)
                io.SetEngine("BP5");
        }
        std::vector<char> metadata;
        std::string openPath = path;
        if (!local)
        {
            const auto cacheDir = std::filesystem::path(cache) / ds.uuid.substr(0, 3) / ds.uuid;
            std::string keyhex;
            if (rep.hasKey)
            {
                auto &k = archive.keys.at(rep.keyIdx);
                if (k.keyHex.empty())
                {
                    Remote connection;
                    k.keyHex = connection.GetKeyFromConnectionManager(k.id);
                }
                if (k.keyHex == "0")
                    throw std::runtime_error("CampaignIndex: no key for dataset " + ds.name);
                keyhex = k.keyHex;
            }
            bool first = true;
            for (auto fileid : rep.files)
            {
                const auto &file = archive.files.at(fileid);
                if (ds.format == FileFormat::ADIOS && file.name == "metadata")
                {
                    metadata.resize(file.lengthOriginal);
                    archive.ReadToMemory(metadata.data(), fileid, keyhex);
                }
                else if (file.name != "profiling.json")
                {
                    helper::CreateDirectory(cacheDir.string());
                    const auto target = cacheDir / std::filesystem::path(file.name).filename();
                    archive.SaveToFile(target.string(), fileid, keyhex);
                    if (ds.format == FileFormat::HDF5 && first)
                        openPath = target.string();
                    first = false;
                }
            }
            if (ds.format == FileFormat::ADIOS && metadata.empty())
                openPath = cacheDir.string();
            io.SetParameter("UUID", ds.uuid);
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
                Params params{{"cache", cacheDir.string()}, {"recheck_metadata", "false"}};
                if (host.defaultProtocol == "https")
                {
                    std::string endpoint =
                        config ? config->endpoint : "https://" + host.longhostname;
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
                io.AddTransport("File", params);
                if (ds.format == FileFormat::HDF5)
                    io.SetParameter("RemoteObjectPath", path);
                else
                {
                    io.SetEngine("BP5");
                    openPath = path;
                }
            }
            else
            {
                io.SetParameter("RemoteDataPath", path);
                io.SetParameter("RemoteHost", host.hostname);
            }
        }
        Engine *engine;
        if (!metadata.empty())
        {
            if (metadata.size() < 8)
                throw std::runtime_error("CampaignIndex: invalid embedded ADIOS metadata");
            std::string type(metadata.data(), 8);
            helper::RightTrim(type);
            io.SetEngine(type);
            engine = &io.Open(openPath, Mode::ReadRandomAccess, m_Comm.Duplicate(), metadata.data(),
                              metadata.size());
        }
        else
            engine = &io.Open(openPath, Mode::ReadRandomAccess, m_Comm.Duplicate());
        return m_DatasetEngines.emplace(key, DatasetEngine{std::move(ownedIO), engine})
            .first->second;
    }
    throw std::runtime_error("CampaignIndex: no readable replica for " + ds.name);
}

CampaignIndexReader::SourceVariable CampaignIndexReader::FindSource(const VariableBase &v,
                                                                    size_t step) const
{
    if (!m_IsOpen)
        throw std::logic_error("CampaignIndex: source lookup on a closed engine");
    if (step >= v.m_AvailableStepsCount)
        throw std::invalid_argument("CampaignIndex: source lookup exceeds indexed steps");
    const auto &entry = m_Entries.at(v.m_Name);
    auto &archive = OpenArchive(entry.archive);
    if (entry.stepRanges)
    {
        const auto &ranges = *entry.stepRanges;
        auto range =
            std::upper_bound(ranges.begin(), ranges.end(), step,
                             [](size_t s, const CampaignIndexStepRange &r) { return s < r.start; });
        // StreamIndex validates that ranges cover every indexed step exactly once.
        --range;
        const auto member = archive.datasets.find(range->dataset);
        if (member == archive.datasets.end() || member->second.name != range->datasetName)
            throw std::runtime_error(
                "CampaignIndex: time-series member no longer matches archive: " +
                range->datasetName);
        auto &source = OpenDataset(entry, range->dataset);
        const auto &variables = source.io->GetVariables();
        auto it = variables.find(entry.name);
        if (it == variables.end() && !entry.name.empty() && entry.name.front() != '/')
            it = variables.find("/" + entry.name);
        if (it == variables.end() || it->second->m_Type != v.m_Type)
            throw std::runtime_error("CampaignIndex: indexed time-series variable missing or type "
                                     "differs in member: " +
                                     v.m_Name);
        auto *actual = it->second.get();
        if (actual->m_AvailableStepsCount < range->steps)
            throw std::runtime_error(
                "CampaignIndex: time-series member has fewer steps than index: " + v.m_Name);
        const size_t localStep = step - range->start;
        return {source.engine, actual, localStep, range->steps - localStep};
    }
    bool found = false;
    for (const auto dataset : ResolveDatasets(archive, entry))
    {
        auto &source = OpenDataset(entry, dataset);
        const auto &variables = source.io->GetVariables();
        auto it = variables.find(entry.name);
        if (it == variables.end() && !entry.name.empty() && entry.name.front() != '/')
            it = variables.find("/" + entry.name);
        if (it == variables.end())
            continue;
        found = true;
        auto *actual = it->second.get();
        if (actual->m_Type != v.m_Type)
            throw std::runtime_error("CampaignIndex: source variable type differs from index: " +
                                     v.m_Name);
        if (step < actual->m_AvailableStepsCount)
            return {source.engine, actual, step, actual->m_AvailableStepsCount - step};
        step -= actual->m_AvailableStepsCount;
    }
    if (!found)
        throw std::runtime_error(
            "CampaignIndex: variable not found with indexed type in dataset: " + v.m_Name);
    throw std::runtime_error("CampaignIndex: source variable has fewer steps than index: " +
                             v.m_Name);
}

template <class T>
std::vector<typename Variable<T>::BPInfo> CampaignIndexReader::BlocksInfo(const Variable<T> &v,
                                                                          size_t step) const
{
    if (step >= v.m_AvailableStepsCount)
        throw std::invalid_argument("CampaignIndex: block inquiry exceeds indexed steps");
    const auto &entry = m_Entries.at(v.m_Name);
    if (entry.format != "ADIOS" && entry.format != "HDF5")
        return {};
    const auto source = FindSource(v, step);
    const auto &actual = static_cast<const Variable<T> &>(*source.variable);
    std::unique_ptr<MinVarInfo> info(source.engine->MinBlocksInfo(actual, source.step));
    std::vector<typename Variable<T>::BPInfo> blocks;
    if (!info)
    {
        // The older typed interface takes absolute source steps.
        const auto steps = source.engine->GetAbsoluteSteps(actual);
        blocks = source.engine->BlocksInfo(actual, steps.at(source.step));
    }
    else
    {
        for (const auto &block : info->BlocksInfo)
        {
            typename Variable<T>::BPInfo result;
            if (info->WasLocalValue)
            {
                result.Shape = {reinterpret_cast<size_t>(info->Shape)};
                result.Start = {reinterpret_cast<size_t>(block.Start)};
                result.Count = {reinterpret_cast<size_t>(block.Count)};
            }
            else
            {
                if (info->Shape && info->Dims)
                    result.Shape.assign(info->Shape, info->Shape + info->Dims);
                if (block.Start && info->Dims)
                    result.Start.assign(block.Start, block.Start + info->Dims);
                if (block.Count && info->Dims)
                    result.Count.assign(block.Count, block.Count + info->Dims);
            }
            result.BlockID = block.BlockID;
            result.WriterID = block.WriterID;
            result.IsValue = info->IsValue;
            result.IsReverseDims = info->IsReverseDims;
            if (info->IsValue)
            {
                if constexpr (std::is_same<T, std::string>::value)
                    result.Value = *static_cast<const char *const *>(block.BufferP);
                else
                    std::memcpy(&result.Value, block.BufferP, sizeof(T));
            }
            else if constexpr (!std::is_same<T, std::string>::value)
            {
                std::memcpy(&result.Min, &block.MinMax.MinUnion, sizeof(T));
                std::memcpy(&result.Max, &block.MinMax.MaxUnion, sizeof(T));
            }
            blocks.push_back(std::move(result));
        }
    }
    for (auto &block : blocks)
        block.Step = step;
    return blocks;
}

template <class T>
MinVarInfo *CampaignIndexReader::FallbackMinBlocksInfo(const Variable<T> &v, size_t step) const
{
    auto &storage = m_BlockStorage[{v.m_Name, step}];
    using Blocks = std::vector<typename Variable<T>::BPInfo>;
    if (!storage.blocks)
    {
        auto blocks = std::make_shared<Blocks>(BlocksInfo(v, step));
        if constexpr (std::is_same<T, std::string>::value)
            for (const auto &block : *blocks)
                storage.strings.push_back(block.Value.c_str());
        storage.blocks = blocks;
    }
    const auto &blocks = *std::static_pointer_cast<Blocks>(storage.blocks);
    auto info = std::make_unique<MinVarInfo>(0, nullptr);
    info->Step = step;
    info->WasLocalValue = false;
    if (!blocks.empty())
    {
        info->IsValue = blocks.front().IsValue;
        info->IsReverseDims = blocks.front().IsReverseDims;
        info->Dims = static_cast<int>(blocks.front().Count.size());
        if (!blocks.front().Shape.empty())
            info->Shape = blocks.front().Shape.data();
    }
    for (size_t i = 0; i < blocks.size(); ++i)
    {
        const auto &block = blocks[i];
        MinBlockInfo result;
        result.BlockID = block.BlockID;
        result.WriterID = block.WriterID;
        result.Start = block.Start.empty() ? nullptr : block.Start.data();
        result.Count = block.Count.empty() ? nullptr : block.Count.data();
        result.MinMax.Init(v.m_Type);
        if constexpr (!std::is_same<T, std::string>::value)
        {
            std::memcpy(&result.MinMax.MinUnion, &block.Min, sizeof(T));
            std::memcpy(&result.MinMax.MaxUnion, &block.Max, sizeof(T));
            if (block.IsValue)
                result.BufferP = const_cast<T *>(&block.Value);
        }
        else
            result.BufferP = &storage.strings[i];
        info->BlocksInfo.push_back(result);
    }
    return info.release();
}

template <class T>
void CampaignIndexReader::ReadDataset(Variable<T> &v, T *data)
{
    const size_t start = m_InStep ? m_CurrentStep : v.m_StepsStart;
    const size_t count = m_InStep ? 1 : v.m_StepsCount;
    if (start > v.m_AvailableStepsCount || count > v.m_AvailableStepsCount - start)
        throw std::invalid_argument("CampaignIndex: step selection exceeds indexed steps");
    size_t completed = 0;
    while (completed < count)
    {
        const auto source = FindSource(v, start + completed);
        auto *actual = static_cast<Variable<T> *>(source.variable);
        const size_t n = std::min(count - completed, source.steps);
        actual->m_SelectionType = v.m_SelectionType;
        actual->m_Start = v.m_Start;
        actual->m_Count = v.m_Count;
        actual->m_BlockID = v.m_BlockID;
        actual->m_MemoryStart = v.m_MemoryStart;
        actual->m_MemoryCount = v.m_MemoryCount;
        actual->m_MemSpace = v.m_MemSpace;
        actual->m_AccuracyRequested = v.m_AccuracyRequested;
        actual->SetStepSelection({source.step, n});
        source.engine->Get(*actual, data, Mode::Sync);
        v.m_AccuracyProvided = actual->m_AccuracyProvided;
        completed += n;
        if (completed == count)
            return;
        const auto &dims = v.m_MemoryCount.empty() ? actual->Count() : v.m_MemoryCount;
        data += n * helper::GetTotalSize(dims);
    }
}

template <class T>
void CampaignIndexReader::GetSync(Variable<T> &v, T *data)
{
    const auto &entry = m_Entries.at(v.m_Name);
    if (entry.format == "ADIOS" || entry.format == "HDF5")
    {
        ReadDataset(v, data);
        return;
    }
    // Parsed text fields and other scalar summaries are cached in the index.
    // Serve them before dispatching raw image/text bytes to the archive.
    if (v.m_SingleValue && v.m_AvailableStepsCount == 1)
    {
        *data = v.m_Value;
        return;
    }
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
        const Variable<T> &v, size_t step) const                                                   \
    {                                                                                              \
        return BlocksInfo(v, step);                                                                \
    }                                                                                              \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>>                                    \
    CampaignIndexReader::DoAllStepsBlocksInfo(const Variable<T> &v) const                          \
    {                                                                                              \
        std::map<size_t, std::vector<typename Variable<T>::BPInfo>> result;                        \
        for (size_t step = 0; step < v.m_AvailableStepsCount; ++step)                              \
            result.emplace(step, BlocksInfo(v, step));                                             \
        return result;                                                                             \
    }                                                                                              \
    std::vector<std::vector<typename Variable<T>::BPInfo>>                                         \
    CampaignIndexReader::DoAllRelativeStepsBlocksInfo(const Variable<T> &v) const                  \
    {                                                                                              \
        std::vector<std::vector<typename Variable<T>::BPInfo>> result;                             \
        for (size_t step = 0; step < v.m_AvailableStepsCount; ++step)                              \
            result.push_back(BlocksInfo(v, step));                                                 \
        return result;                                                                             \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

} // namespace engine
} // namespace core
} // namespace adios2
