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
{
    m_IsRowMajor = isRowMajor;
    m_IsLittleEndian = isLittleEndian;
    m_ContiguousMajor = contiguousMajor;
    m_DeferredRequestsToSend =
        std::make_shared<std::unordered_map<std::string, std::vector<char>>>();
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
    std::vector<char> metacbor;
    nlohmann::json::to_msgpack(m_MetadataJson, metacbor);
    size_t metasize = metacbor.size();
    (reinterpret_cast<uint64_t *>(m_LocalBuffer->data()))[0] =
        m_LocalBuffer->size();
    (reinterpret_cast<uint64_t *>(m_LocalBuffer->data()))[1] = metasize;
    m_LocalBuffer->resize(m_LocalBuffer->size() + metasize);
    std::memcpy(m_LocalBuffer->data() + m_LocalBuffer->size() - metasize,
                metacbor.data(), metasize);
    return m_LocalBuffer;
}

std::shared_ptr<std::vector<char>>
DataManSerializer::GetAggregatedMetadata(const MPI_Comm mpiComm)
{

    std::vector<char> localJsonCbor;
    nlohmann::json::to_msgpack(m_MetadataJson, localJsonCbor);
    unsigned int size = localJsonCbor.size();
    unsigned int maxSize;
    MPI_Allreduce(&size, &maxSize, 1, MPI_UNSIGNED, MPI_MAX, mpiComm);
    maxSize += sizeof(uint64_t);
    localJsonCbor.resize(maxSize, '\0');
    *(reinterpret_cast<uint64_t *>(localJsonCbor.data() +
                                   localJsonCbor.size()) -
      1) = size;

    int mpiSize;
    int mpiRank;
    MPI_Comm_size(mpiComm, &mpiSize);
    MPI_Comm_rank(mpiComm, &mpiRank);

    std::vector<char> globalJsonStr(mpiSize * maxSize);
    helper::GatherArrays(localJsonCbor.data(), maxSize, globalJsonStr.data(),
                         mpiComm);

    nlohmann::json globalMetadata;
    std::shared_ptr<std::vector<char>> globalJsonCbor = nullptr;
    if (mpiRank == 0)
    {
        for (int i = 0; i < mpiSize; ++i)
        {
            size_t deserializeSize =
                *(reinterpret_cast<uint64_t *>(globalJsonStr.data() +
                                               (i + 1) * maxSize) -
                  1);
            nlohmann::json metaj = nlohmann::json::from_msgpack(
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
        globalJsonCbor = std::make_shared<std::vector<char>>();
        nlohmann::json::to_msgpack(globalMetadata, *globalJsonCbor);
    }

    return globalJsonCbor;
}

void DataManSerializer::PutAggregatedMetadata(
    const MPI_Comm mpiComm, std::shared_ptr<std::vector<char>> data)
{
    int mpiSize;
    int mpiRank;
    MPI_Comm_size(mpiComm, &mpiSize);
    MPI_Comm_rank(mpiComm, &mpiRank);
    helper::BroadcastVector(*data, mpiComm);

    nlohmann::json metaJ =
        nlohmann::json::from_msgpack(data->data(), data->size());
    JsonToDataManVarMap(metaJ, nullptr);

    if (m_Verbosity >= 5)
    {
        if (mpiRank == 0)
        {
            std::cout << "DataManSerializer::PutAggregatedMetadata: "
                      << std::endl;
            std::cout << metaJ.dump(4) << std::endl;
        }
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
        if (type == "int" || type == "long" || type == "int32_t" ||
            type == "int64_t" || type == "float" || type == "double")
        {
            if (count.size() <= 3)
            {
                return true;
            }
        }
    }
    else if (method == "sz")
    {
        if (type == "float" || type == "double")
        {
            if (count.size() <= 5)
            {
                return true;
            }
        }
    }
    else if (method == "bzip2")
    {
        if (type == "int" || type == "long" || type == "int32_t" ||
            type == "int64_t" || type == "float" || type == "double")
        {
            return true;
        }
    }
    return false;
}

void DataManSerializer::PutAttributes(core::IO &io, const int rank)
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
        ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
    }
}

void DataManSerializer::JsonToDataManVarMap(
    nlohmann::json &metaJ, std::shared_ptr<std::vector<char>> pack)
{

    // the mutex has to be locked here through the entire function. Otherwise
    // reader engine could get incomplete step metadata. This function only
    // deals with JSON metadata and data buffer already in allocated shared
    // pointers, so it should be cheap to lock.
    std::lock_guard<std::mutex> l(m_Mutex);

    for (auto stepMapIt = metaJ.begin(); stepMapIt != metaJ.end(); ++stepMapIt)
    {
        if (stepMapIt.key() == "G" || stepMapIt.key() == "A")
        {
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
    nlohmann::json j =
        nlohmann::json::from_msgpack(data->data() + metaPosition, metaSize);

    JsonToDataManVarMap(j, data);

    return 0;
}

void DataManSerializer::Erase(const size_t step, const bool allPreviousSteps,
                              const size_t reserveSteps)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    if (allPreviousSteps)
    {
        for (size_t i = 0; i <= step; ++i)
        {
            m_DataManVarMap.erase(i);
        }
    }
    else
    {
        m_DataManVarMap.erase(step);
    }
}

const std::unordered_map<
    size_t, std::shared_ptr<std::vector<DataManSerializer::DataManVar>>>
DataManSerializer::GetMetaData()
{
    std::lock_guard<std::mutex> l(m_Mutex);
    // This meta data map is supposed to be very light weight to return because
    // 1) it only holds shared pointers, and 2) the old steps are removed
    // regularly by the engine.
    return m_DataManVarMap;
}

std::shared_ptr<const std::vector<DataManSerializer::DataManVar>>
DataManSerializer::GetMetaData(const size_t step)
{
    std::lock_guard<std::mutex> l(m_Mutex);
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
    std::lock_guard<std::mutex> l(m_Mutex);
    for (const auto &j : m_GlobalVars)
    {
        const std::string type(j["Y"].get<std::string>());
        if (type == "unknown")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        if (j["V"].get<bool>())                                                \
        {                                                                      \
            io.DefineAttribute<T>(j["N"].get<std::string>(), j["G"].get<T>()); \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            io.DefineAttribute<T>(j["N"].get<std::string>(),                   \
                                  j["G"].get<std::vector<T>>().data(),         \
                                  j["G"].get<std::vector<T>>().size());        \
        }                                                                      \
    }
        ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
    }
}

int DataManSerializer::PutDeferredRequest(const std::string &variable,
                                          const size_t step, const Dims &start,
                                          const Dims &count, void *data)
{
    std::shared_ptr<std::vector<DataManVar>> varVec;

    m_Mutex.lock();
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
    m_Mutex.unlock();

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
        j["O"] = start;
        j["C"] = count;
        j["T"] = step;
    }

    for (const auto &i : jmap)
    {
        std::vector<char> cbor;
        nlohmann::json::to_msgpack(i.second, cbor);
        (*m_DeferredRequestsToSend)[i.first] = cbor;
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

std::shared_ptr<std::vector<char>> DataManSerializer::GenerateReply(
    std::shared_ptr<const std::vector<char>> request, bool autoEraseOldSteps,
    size_t reserveSteps)
{
    auto replyMetaJ = std::make_shared<nlohmann::json>();
    auto replyLocalBuffer = std::make_shared<std::vector<char>>();

    nlohmann::json metaj;
    try
    {
        metaj = nlohmann::json::from_msgpack(request->data(), request->size());
    }
    catch (std::exception &e)
    {
        std::cout << "DataManSerializer received staging request but failed to "
                     "deserialize"
                  << std::endl;
        return replyLocalBuffer;
    }

    replyLocalBuffer->resize(sizeof(uint64_t) * 2);

    size_t stepToErase = std::numeric_limits<size_t>::max();

    for (const auto &req : metaj)
    {
        std::string variable = req["N"].get<std::string>();
        Dims start = req["O"].get<Dims>();
        Dims count = req["C"].get<Dims>();
        size_t step = req["T"].get<size_t>();
        if (step < stepToErase)
        {
            stepToErase = step;
        }

        std::shared_ptr<std::vector<DataManVar>> varVec;

        {
            std::lock_guard<std::mutex> l(m_Mutex);
            auto itVarVec = m_DataManVarMap.find(step);
            if (itVarVec == m_DataManVarMap.end())
            {
                return replyLocalBuffer;
            }
            else
            {
                varVec = itVarVec->second;
                if (varVec == nullptr)
                {
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
                    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

                    std::vector<char> metacbor;
                    nlohmann::json::to_msgpack(*replyMetaJ, metacbor);
                    size_t metasize = metacbor.size();
                    (reinterpret_cast<uint64_t *>(
                        replyLocalBuffer->data()))[0] =
                        replyLocalBuffer->size();
                    (reinterpret_cast<uint64_t *>(
                        replyLocalBuffer->data()))[1] = metasize;
                    replyLocalBuffer->resize(replyLocalBuffer->size() +
                                             metasize);
                    std::memcpy(replyLocalBuffer->data() +
                                    replyLocalBuffer->size() - metasize,
                                metacbor.data(), metasize);
                }
            }
        }
    }
    if (autoEraseOldSteps)
    {
        if (stepToErase - reserveSteps - 1 >= 0)
            Erase(stepToErase - reserveSteps - 1, true);
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

int64_t DataManSerializer::MinStep()
{
    std::lock_guard<std::mutex> l(m_Mutex);
    int64_t minStep = std::numeric_limits<int64_t>::max();
    for (const auto &i : m_DataManVarMap)
    {
        if (minStep > i.first)
        {
            minStep = i.first;
        }
    }
    return minStep;
}

int64_t DataManSerializer::Steps()
{
    std::lock_guard<std::mutex> l(m_Mutex);
    return m_DataManVarMap.size();
}

} // namespace format
} // namespace adios2
