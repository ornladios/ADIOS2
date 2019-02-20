/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManSerializer.cpp Serializer class for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#include "DataManSerializer.tcc"

#include "adios2/helper/adiosMPIFunctions.h"

#include <cstring>
#include <iostream>

namespace adios2
{
namespace format
{

DataManSerializer::DataManSerializer(bool isRowMajor,
                                     const bool contiguousMajor,
                                     bool isLittleEndian)
: m_IsRowMajor(isRowMajor), m_IsLittleEndian(isLittleEndian),
  m_ContiguousMajor(contiguousMajor),
  m_DeferredRequestsToSend(
      std::make_shared<std::unordered_map<std::string, std::vector<char>>>())

{
    New(1024);
}

void DataManSerializer::New(size_t size)
{
    // make a new shared object each time because the old shared object could
    // still be alive and needed somewhere in the workflow, for example the
    // queue in transport manager. It will be automatically released when the
    // entire workflow finishes using it.
    m_MetadataJson = nullptr;
    m_LocalBuffer = std::make_shared<std::vector<char>>();
    m_LocalBuffer->reserve(size);
    m_LocalBuffer->resize(sizeof(uint64_t) * 2);
}

const std::shared_ptr<std::vector<char>> DataManSerializer::GetLocalPack()
{
    std::vector<char> metapack = SerializeJson(m_MetadataJson);
    size_t metasize = metapack.size();
    (reinterpret_cast<uint64_t *>(m_LocalBuffer->data()))[0] =
        m_LocalBuffer->size();
    (reinterpret_cast<uint64_t *>(m_LocalBuffer->data()))[1] = metasize;
    m_LocalBuffer->resize(m_LocalBuffer->size() + metasize);
    std::memcpy(m_LocalBuffer->data() + m_LocalBuffer->size() - metasize,
                metapack.data(), metasize);
    return m_LocalBuffer;
}

std::shared_ptr<std::vector<char>>
DataManSerializer::GetAggregatedMetadata(const MPI_Comm mpiComm)
{
    int mpiSize;
    int mpiRank;
    MPI_Comm_size(mpiComm, &mpiSize);
    MPI_Comm_rank(mpiComm, &mpiRank);

    std::vector<char> localJsonPack = SerializeJson(m_MetadataJson);
    unsigned int size = localJsonPack.size();
    unsigned int maxSize;
    MPI_Allreduce(&size, &maxSize, 1, MPI_UNSIGNED, MPI_MAX, mpiComm);
    maxSize += sizeof(uint64_t);
    localJsonPack.resize(maxSize, '\0');
    *(reinterpret_cast<uint64_t *>(localJsonPack.data() +
                                   localJsonPack.size()) -
      1) = size;

    std::vector<char> globalJsonStr(mpiSize * maxSize);
    MPI_Allgather(localJsonPack.data(), maxSize, MPI_CHAR, globalJsonStr.data(),
                  maxSize, MPI_CHAR, mpiComm);

    nlohmann::json globalMetadata;
    std::shared_ptr<std::vector<char>> globalJsonPack = nullptr;

    for (int i = 0; i < mpiSize; ++i)
    {
        size_t deserializeSize =
            *(reinterpret_cast<uint64_t *>(globalJsonStr.data() +
                                           (i + 1) * maxSize) -
              1);
        nlohmann::json metaj = DeserializeJson(
            globalJsonStr.data() + i * maxSize, deserializeSize);
        for (auto stepMapIt = metaj.begin(); stepMapIt != metaj.end();
             ++stepMapIt)
        {
            for (auto rankMapIt = stepMapIt.value().begin();
                 rankMapIt != stepMapIt.value().end(); ++rankMapIt)
            {
                globalMetadata[stepMapIt.key()][rankMapIt.key()] =
                    rankMapIt.value();
            }
        }
    }
    globalJsonPack = std::make_shared<std::vector<char>>(
        std::move(SerializeJson(globalMetadata)));

    if (m_Verbosity >= 1)
    {
        if (mpiRank == 0)
        {
            if (globalJsonPack->empty())
            {
                std::cout << "DataManSerializer::GetAggregatedMetadata "
                             "resulted in empty json pack"
                          << std::endl;
            }
        }
    }
    return globalJsonPack;
}

void DataManSerializer::PutAggregatedMetadata(
    const MPI_Comm mpiComm, std::shared_ptr<std::vector<char>> data)
{
    if (data->empty())
    {
        Log(1, "DataManSerializer::PutAggregatedMetadata tries to deserialize "
               "an empty json pack",
            true, true);
    }

    nlohmann::json metaJ = DeserializeJson(data->data(), data->size());
    JsonToDataManVarMap(metaJ, nullptr);

    if (m_Verbosity >= 100)
    {
        std::cout << "DataManSerializer::PutAggregatedMetadata: " << std::endl;
        std::cout << metaJ.dump(4) << std::endl;
    }
}

std::shared_ptr<std::vector<char>> DataManSerializer::EndSignal(size_t step)
{
    nlohmann::json j;
    j["FinalStep"] = step;
    std::string s = j.dump() + '\0';
    std::shared_ptr<std::vector<char>> c =
        std::make_shared<std::vector<char>>(s.size());
    std::memcpy(c->data(), s.c_str(), s.size());
    return c;
}

bool DataManSerializer::IsCompressionAvailable(const std::string &method,
                                               const std::string &type,
                                               const Dims &count)
{
    if (method == "zfp")
    {
        if (type == helper::GetType<int32_t>() ||
            type == helper::GetType<int64_t>() ||
            type == helper::GetType<float>() ||
            type == helper::GetType<double>())
        {
            if (count.size() <= 3)
            {
                return true;
            }
        }
    }
    else if (method == "sz")
    {
        if (type == helper::GetType<float>() ||
            type == helper::GetType<double>())
        {
            if (count.size() <= 5)
            {
                return true;
            }
        }
    }
    else if (method == "bzip2")
    {
        return true;
    }
    return false;
}

void DataManSerializer::PutAttributes(core::IO &io, const int rank)
{
    if (rank == 0)
    {
        const auto attributesDataMap = io.GetAttributesDataMap();
        for (const auto &attributePair : attributesDataMap)
        {
            const std::string name(attributePair.first);
            const std::string type(attributePair.second.first);
            if (type == "unknown")
            {
            }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        core::Attribute<T> &attribute = *io.InquireAttribute<T>(name);         \
        PutAttribute(attribute, rank);                                         \
    }
            ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
        }
    }
}

void DataManSerializer::JsonToDataManVarMap(
    nlohmann::json &metaJ, std::shared_ptr<std::vector<char>> pack)
{

    // the mutex has to be locked here through the entire function. Otherwise
    // reader engine could get incomplete step metadata. This function only
    // deals with JSON metadata and data buffer already in allocated shared
    // pointers, so it should be cheap to lock.
    std::lock_guard<std::mutex> l(m_DataManVarMapMutex);

    for (auto stepMapIt = metaJ.begin(); stepMapIt != metaJ.end(); ++stepMapIt)
    {
        if (stepMapIt.key() == "G" || stepMapIt.key() == "A")
        {
            std::lock_guard<std::mutex> l(m_GlobalVarsMutex);
            for (const auto &rankVec : stepMapIt.value())
            {
                for (const auto &gVar : rankVec)
                {
                    m_GlobalVars[gVar["N"].get<std::string>()] = gVar;
                }
            }
            continue;
        }

        for (auto rankMapIt = stepMapIt.value().begin();
             rankMapIt != stepMapIt.value().end(); ++rankMapIt)
        {
            for (const auto &varBlock : rankMapIt.value())
            {
                DataManVar var;
                try
                {
                    // compulsory properties
                    var.step = stoull(stepMapIt.key());
                    var.name = varBlock["N"].get<std::string>();
                    var.start = varBlock["O"].get<Dims>();
                    var.count = varBlock["C"].get<Dims>();
                    var.size = varBlock["I"].get<size_t>();
                    var.rank = stoi(rankMapIt.key());
                    var.address = varBlock["A"].get<std::string>();

                    // optional properties

                    auto itJson = varBlock.find("D");
                    if (itJson != varBlock.end())
                    {
                        var.doid = itJson->get<std::string>();
                    }

                    itJson = varBlock.find("M");
                    if (itJson != varBlock.end())
                    {
                        var.isRowMajor = itJson->get<bool>();
                    }

                    itJson = varBlock.find("E");
                    if (itJson != varBlock.end())
                    {
                        var.isLittleEndian = itJson->get<bool>();
                    }

                    itJson = varBlock.find("Y");
                    if (itJson != varBlock.end())
                    {
                        var.type = itJson->get<std::string>();
                    }

                    itJson = varBlock.find("S");
                    if (itJson != varBlock.end())
                    {
                        var.shape = itJson->get<Dims>();
                    }

                    var.position = varBlock["P"].get<size_t>();
                    var.buffer = pack;

                    auto it = varBlock.find("Z");
                    if (it != varBlock.end())
                    {
                        var.compression = it->get<std::string>();
                    }

                    for (auto i = varBlock.begin(); i != varBlock.end(); ++i)
                    {
                        auto pos = i.key().find(":");
                        if (pos != std::string::npos)
                        {
                            var.params[i.key().substr(pos + 1)] = i.value();
                        }
                    }

                    if (m_DataManVarMap[var.step] == nullptr)
                    {
                        m_DataManVarMap[var.step] =
                            std::make_shared<std::vector<DataManVar>>();
                    }
                    m_DataManVarMap[var.step]->emplace_back(std::move(var));
                }
                catch (std::exception &e)
                {
                    std::cout << e.what() << std::endl;
                }
            }
        }
    }
    if (m_Verbosity >= 5)
    {
        std::cout
            << "DataManSerializer::JsonToDataManVarMap Total buffered steps = "
            << m_DataManVarMap.size() << std::endl;
    }
}

int DataManSerializer::PutPack(const std::shared_ptr<std::vector<char>> data)
{
    if (data->size() == 0)
    {
        return -1;
    }

    // check if is control signal
    if (data->size() < 128)
    {
        try
        {
            nlohmann::json metaj = nlohmann::json::parse(data->data());
            size_t finalStep = metaj["FinalStep"];
            return finalStep;
        }
        catch (std::exception)
        {
        }
    }

    // if not control signal then go through standard deserialization

    uint64_t metaPosition =
        (reinterpret_cast<const uint64_t *>(data->data()))[0];
    uint64_t metaSize = (reinterpret_cast<const uint64_t *>(data->data()))[1];
    nlohmann::json j = DeserializeJson(data->data() + metaPosition, metaSize);

    JsonToDataManVarMap(j, data);

    return 0;
}

void DataManSerializer::Erase(const size_t step, const bool allPreviousSteps)
{
    std::lock_guard<std::mutex> l1(m_DataManVarMapMutex);
    if (allPreviousSteps)
    {
        std::vector<std::unordered_map<
            size_t, std::shared_ptr<std::vector<DataManVar>>>::iterator>
            its;
        for (auto it = m_DataManVarMap.begin(); it != m_DataManVarMap.end();
             ++it)
        {
            if (it->first < step)
            {
                its.push_back(it);
            }
        }
        for (auto it : its)
        {
            Log(5, "DataManSerializer::Erase() trying to erase step " +
                       std::to_string(it->first) +
                       ". This is one of multiple steps being erased.",
                true, true);
            m_DataManVarMap.erase(it);
        }
    }
    else
    {
        Log(5, "DataManSerializer::Erase() trying to erase step " +
                   std::to_string(step),
            true, true);
        m_DataManVarMap.erase(step);
    }
}

const std::unordered_map<
    size_t, std::shared_ptr<std::vector<DataManSerializer::DataManVar>>>
DataManSerializer::GetMetaData()
{
    std::lock_guard<std::mutex> l(m_DataManVarMapMutex);
    // This meta data map is supposed to be very light weight to return because
    // 1) it only holds shared pointers, and 2) the old steps are removed
    // regularly by the engine.
    return m_DataManVarMap;
}

std::shared_ptr<const std::vector<DataManSerializer::DataManVar>>
DataManSerializer::GetMetaData(const size_t step)
{
    std::lock_guard<std::mutex> l(m_DataManVarMapMutex);
    const auto &i = m_DataManVarMap.find(step);
    if (i != m_DataManVarMap.end())
    {
        return m_DataManVarMap[step];
    }
    else
    {
        return nullptr;
    }
}

void DataManSerializer::GetAttributes(core::IO &io)
{
    std::lock_guard<std::mutex> l(m_GlobalVarsMutex);
    const auto attributesDataMap = io.GetAttributesDataMap();
    for (const auto &j : m_GlobalVars)
    {
        const std::string type(j["Y"].get<std::string>());
        if (type == "unknown")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        auto it = attributesDataMap.find(j["N"].get<std::string>());           \
        if (it == attributesDataMap.end())                                     \
        {                                                                      \
            if (j["V"].get<bool>())                                            \
            {                                                                  \
                io.DefineAttribute<T>(j["N"].get<std::string>(),               \
                                      j["G"].get<T>());                        \
            }                                                                  \
            else                                                               \
            {                                                                  \
                io.DefineAttribute<T>(j["N"].get<std::string>(),               \
                                      j["G"].get<std::vector<T>>().data(),     \
                                      j["G"].get<std::vector<T>>().size());    \
            }                                                                  \
        }                                                                      \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
}

int DataManSerializer::PutDeferredRequest(const std::string &variable,
                                          const size_t step, const Dims &start,
                                          const Dims &count, void *data)
{

    std::shared_ptr<std::vector<DataManVar>> varVec;

    m_DataManVarMapMutex.lock();
    auto stepVecIt = m_DataManVarMap.find(step);
    if (stepVecIt == m_DataManVarMap.end())
    {
        // aggregated metadata does not have this step
        std::cout << "aggregated metadata does not have Step " << step
                  << std::endl;
        return -1;
    }
    else
    {
        varVec = stepVecIt->second;
    }
    m_DataManVarMapMutex.unlock();

    std::unordered_map<std::string, nlohmann::json> jmap;

    for (const auto &var : *varVec)
    {
        if (var.name == variable)
        {
            if (var.start.size() != start.size() ||
                var.count.size() != count.size() ||
                start.size() != count.size())
            {
                // requested shape does not match metadata
                continue;
            }
            for (size_t i = 0; i < start.size(); ++i)
            {
                if (start[i] > var.start[i] + var.count[i] ||
                    start[i] + count[i] < var.start[i])
                {
                    // current iteration does not have the desired part
                    continue;
                }
            }
        }

        jmap[var.address].emplace_back();
        nlohmann::json &j = jmap[var.address].back();
        j["N"] = variable;
        j["O"] = var.start;
        j["C"] = var.count;
        j["T"] = step;
    }

    for (const auto &i : jmap)
    {
        (*m_DeferredRequestsToSend)[i.first] = SerializeJson(i.second);
    }

    return 0;
}

std::shared_ptr<std::unordered_map<std::string, std::vector<char>>>
DataManSerializer::GetDeferredRequest()
{
    auto t = m_DeferredRequestsToSend;
    m_DeferredRequestsToSend =
        std::make_shared<std::unordered_map<std::string, std::vector<char>>>();
    return t;
}

std::shared_ptr<std::vector<char>>
DataManSerializer::GenerateReply(const std::vector<char> &request, size_t &step)
{
    auto replyMetaJ = std::make_shared<nlohmann::json>();
    auto replyLocalBuffer =
        std::make_shared<std::vector<char>>(sizeof(uint64_t) * 2);

    nlohmann::json metaj;
    try
    {
        metaj = DeserializeJson(request.data(), request.size());
    }
    catch (std::exception &e)
    {
        Log(1, "DataManSerializer::GenerateReply() received staging request "
               "but failed to deserialize due to " +
                   std::string(e.what()),
            true, true);
        step = -1;
        return replyLocalBuffer;
    }

    for (const auto &req : metaj)
    {
        std::string variable = req["N"].get<std::string>();
        Dims start = req["O"].get<Dims>();
        Dims count = req["C"].get<Dims>();
        step = req["T"].get<size_t>();

        std::shared_ptr<std::vector<DataManVar>> varVec;

        {
            std::lock_guard<std::mutex> l(m_DataManVarMapMutex);
            auto itVarVec = m_DataManVarMap.find(step);
            if (itVarVec == m_DataManVarMap.end())
            {
                Log(1, "DataManSerializer::GenerateReply() received staging "
                       "request but DataManVarMap does not have Step " +
                           std::to_string(step),
                    true, true);
                if (m_Verbosity >= 1)
                {
                    std::string msg =
                        "DataManSerializer::GenerateReply() current steps are ";
                    for (auto s : m_DataManVarMap)
                    {
                        msg += std::to_string(s.first) + ", ";
                    }
                    Log(1, msg, true, true);
                }
                return replyLocalBuffer;
            }
            else
            {
                varVec = itVarVec->second;
                if (varVec == nullptr)
                {
                    Log(1, "DataManSerializer::GenerateReply() received "
                           "staging request but DataManVarMap contains a "
                           "nullptr for Step " +
                               std::to_string(step),
                        true, true);
                    return replyLocalBuffer;
                }
            }
        }
        for (const auto &var : *varVec)
        {
            if (var.name == variable)
            {
                Dims ovlpStart, ovlpCount;
                bool ovlp = CalculateOverlap(var.start, var.count, start, count,
                                             ovlpStart, ovlpCount);
                if (ovlp)
                {
                    std::vector<char> tmpBuffer;
                    if (var.type == "compound")
                    {
                        throw("Compound type is not supported yet.");
                    }
#define declare_type(T)                                                        \
    else if (var.type == helper::GetType<T>())                                 \
    {                                                                          \
        tmpBuffer.reserve(std::accumulate(ovlpCount.begin(), ovlpCount.end(),  \
                                          sizeof(T),                           \
                                          std::multiplies<size_t>()));         \
        GetVar(reinterpret_cast<T *>(tmpBuffer.data()), variable, ovlpStart,   \
               ovlpCount, step);                                               \
        PutVar(reinterpret_cast<T *>(tmpBuffer.data()), variable, var.shape,   \
               ovlpStart, ovlpCount, ovlpStart, ovlpCount, var.doid, step,     \
               var.rank, var.address, Params(), replyLocalBuffer, replyMetaJ); \
    }
                    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

                    std::vector<char> metapack = SerializeJson(*replyMetaJ);
                    size_t metasize = metapack.size();
                    (reinterpret_cast<uint64_t *>(
                        replyLocalBuffer->data()))[0] =
                        replyLocalBuffer->size();
                    (reinterpret_cast<uint64_t *>(
                        replyLocalBuffer->data()))[1] = metasize;
                    replyLocalBuffer->resize(replyLocalBuffer->size() +
                                             metasize);
                    std::memcpy(replyLocalBuffer->data() +
                                    replyLocalBuffer->size() - metasize,
                                metapack.data(), metasize);
                }
            }
        }
    }
    if (m_Verbosity >= 1)
    {
        if (replyLocalBuffer->size() <= 16)
        {
            std::cout << "DataManSerializer::GenerateReply returns a buffer "
                         "with size "
                      << replyLocalBuffer->size()
                      << ", which means no data is contained in the buffer. "
                         "This will cause the deserializer to unpack incorrect "
                         "data for Step "
                      << step << "." << std::endl;
        }
    }
    return replyLocalBuffer;
}

bool DataManSerializer::CalculateOverlap(const Dims &inStart,
                                         const Dims &inCount,
                                         const Dims &outStart,
                                         const Dims &outCount, Dims &ovlpStart,
                                         Dims &ovlpCount)
{
    if (inStart.size() != inCount.size() ||
        outStart.size() != outCount.size() || inStart.size() != outStart.size())
    {
        return false;
    }
    if (ovlpStart.size() != inStart.size())
    {
        ovlpStart.resize(inStart.size());
    }
    if (ovlpCount.size() != inStart.size())
    {
        ovlpCount.resize(inStart.size());
    }
    for (size_t i = 0; i < inStart.size(); ++i)
    {
        if (inStart[i] + inCount[i] < outStart[i])
        {
            return false;
        }
        if (outStart[i] + outCount[i] < inStart[i])
        {
            return false;
        }
        if (inStart[i] < outStart[i])
        {
            ovlpStart[i] = outStart[i];
        }
        else
        {
            ovlpStart[i] = inStart[i];
        }
        if (inStart[i] + inCount[i] < outStart[i] + outCount[i])
        {
            ovlpCount[i] = inStart[i] + inCount[i] - ovlpStart[i];
        }
        else
        {
            ovlpCount[i] = outStart[i] + outCount[i] - ovlpStart[i];
        }
    }
    return true;
}

size_t DataManSerializer::MinStep()
{
    std::lock_guard<std::mutex> l(m_DataManVarMapMutex);
    size_t minStep = std::numeric_limits<size_t>::max();
    for (const auto &i : m_DataManVarMap)
    {
        if (minStep > i.first)
        {
            minStep = i.first;
        }
    }
    return minStep;
}

size_t DataManSerializer::Steps()
{
    std::lock_guard<std::mutex> l(m_DataManVarMapMutex);
    return m_DataManVarMap.size();
}

std::vector<char>
DataManSerializer::SerializeJson(const nlohmann::json &message)
{
    std::vector<char> pack;
    if (m_UseJsonSerialization == "msgpack")
    {
        nlohmann::json::to_msgpack(message, pack);
    }
    else if (m_UseJsonSerialization == "cbor")
    {
        nlohmann::json::to_cbor(message, pack);
    }
    else if (m_UseJsonSerialization == "ubjson")
    {
        nlohmann::json::to_ubjson(message, pack);
    }
    else if (m_UseJsonSerialization == "string")
    {
        std::string pack_str = message.dump();
        pack.resize(pack_str.size() + 1);
        std::memcpy(pack.data(), pack_str.data(), pack_str.size());
        pack.back() = '\0';
        if (m_Verbosity >= 5)
        {
            std::cout << "DataManSerializer::SerializeJson Json = ";
            std::cout << nlohmann::json::parse(pack).dump(4);
            std::cout << std::endl;
        }
    }
    else
    {
        throw(std::invalid_argument(m_UseJsonSerialization +
                                    " is not a valid method. DataManSerializer "
                                    "only uses string, msgpack, cbor or "
                                    "ubjson"));
    }
    return pack;
}

nlohmann::json DataManSerializer::DeserializeJson(const char *start,
                                                  size_t size)
{
    nlohmann::json message;
    if (m_UseJsonSerialization == "msgpack")
    {
        message = nlohmann::json::from_msgpack(start, size);
    }
    else if (m_UseJsonSerialization == "cbor")
    {
        message = nlohmann::json::from_cbor(start, size);
    }
    else if (m_UseJsonSerialization == "ubjson")
    {
        message = nlohmann::json::from_ubjson(start, size);
    }
    else if (m_UseJsonSerialization == "string")
    {
        message = nlohmann::json::parse(start);
    }
    else
    {
        throw(std::invalid_argument(m_UseJsonSerialization +
                                    " is not a valid method. DataManSerializer "
                                    "only uses string, msgpack, cbor or "
                                    "ubjson"));
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "DataManSerializer::DeserializeJson Json = "
                  << message.dump() << std::endl;
    }
    return message;
}

void DataManSerializer::Log(const int level, const std::string &message,
                            const bool mpi, const bool endline)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (m_Verbosity >= level)
    {
        if (mpi)
        {
            std::cout << "[Rank " << rank << "] ";
        }
        std::cout << message;
        if (endline)
        {
            std::cout << std::endl;
        }
    }
}

} // namespace format
} // namespace adios2
