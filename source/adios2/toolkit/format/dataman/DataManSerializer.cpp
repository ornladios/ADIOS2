/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManSerializer.cpp Serializer class for DataMan streaming format
 *
 *  Created on: May 11, 2018
 *      Author: Jason Wang
 */

#include "DataManSerializer.h"
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
  m_DeferredRequestsToSend(std::make_shared<DeferredRequestMap>())

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

VecPtr DataManSerializer::GetLocalPack()
{
    auto metapack = SerializeJson(m_MetadataJson);
    size_t metasize = metapack->size();
    (reinterpret_cast<uint64_t *>(m_LocalBuffer->data()))[0] =
        m_LocalBuffer->size();
    (reinterpret_cast<uint64_t *>(m_LocalBuffer->data()))[1] = metasize;
    m_LocalBuffer->resize(m_LocalBuffer->size() + metasize);
    std::memcpy(m_LocalBuffer->data() + m_LocalBuffer->size() - metasize,
                metapack->data(), metasize);
    return m_LocalBuffer;
}

void DataManSerializer::AggregateMetadata(const MPI_Comm mpiComm)
{
    int mpiSize;
    int mpiRank;
    MPI_Comm_size(mpiComm, &mpiSize);
    MPI_Comm_rank(mpiComm, &mpiRank);

    auto localJsonPack = SerializeJson(m_MetadataJson);
    unsigned int size = localJsonPack->size();
    unsigned int maxSize;
    MPI_Allreduce(&size, &maxSize, 1, MPI_UNSIGNED, MPI_MAX, mpiComm);
    maxSize += sizeof(uint64_t);
    localJsonPack->resize(maxSize, '\0');
    *(reinterpret_cast<uint64_t *>(localJsonPack->data() +
                                   localJsonPack->size()) -
      1) = size;

    std::vector<char> globalJsonStr(mpiSize * maxSize);
    MPI_Allgather(localJsonPack->data(), maxSize, MPI_CHAR,
                  globalJsonStr.data(), maxSize, MPI_CHAR, mpiComm);

    nlohmann::json aggMetadata;

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
                aggMetadata[stepMapIt.key()][rankMapIt.key()] =
                    rankMapIt.value();
            }
        }
    }

    m_AggregatedMetadataJsonMutex.lock();

    for (auto stepMapIt = aggMetadata.begin(); stepMapIt != aggMetadata.end();
         ++stepMapIt)
    {
        m_AggregatedMetadataJson[stepMapIt.key()] = stepMapIt.value();
    }

    m_AggregatedMetadataJsonMutex.unlock();
}

VecPtr DataManSerializer::GetAggregatedMetadataPack(const int64_t step)
{

    VecPtr ret = nullptr;

    m_AggregatedMetadataJsonMutex.lock();

    auto it = m_AggregatedMetadataJson.find(std::to_string(step));

    if (it != m_AggregatedMetadataJson.end())
    {
        nlohmann::json retJ;
        retJ[std::to_string(step)] = *it;
        ret = SerializeJson(retJ);
    }
    else
    {
        if (step == -1) // getting the earliest step
        {
            int64_t min = std::numeric_limits<int64_t>::max();
            for (auto stepMapIt = m_AggregatedMetadataJson.begin();
                 stepMapIt != m_AggregatedMetadataJson.end(); ++stepMapIt)
            {
                int64_t step = stoll(stepMapIt.key());
                if (min > step)
                {
                    min = step;
                }
            }
            nlohmann::json retJ;
            retJ[std::to_string(min)] =
                m_AggregatedMetadataJson[std::to_string(min)];
            ret = SerializeJson(retJ);
        }
        else if (step == -2) // getting the latest step
        {
            int64_t max = std::numeric_limits<int64_t>::min();
            for (auto stepMapIt = m_AggregatedMetadataJson.begin();
                 stepMapIt != m_AggregatedMetadataJson.end(); ++stepMapIt)
            {
                int64_t step = stoll(stepMapIt.key());
                if (max < step)
                {
                    max = step;
                }
            }
            nlohmann::json retJ;
            retJ[std::to_string(max)] =
                m_AggregatedMetadataJson[std::to_string(max)];
            ret = SerializeJson(retJ);
        }
        else if (step == -3) // getting static variables
        {
            ret = SerializeJson(m_StaticDataJson);
        }
        else if (step == -4) // getting all steps
        {
            ret = SerializeJson(m_AggregatedMetadataJson);
        }
    }

    m_AggregatedMetadataJsonMutex.unlock();

    return ret;
}

void DataManSerializer::PutAggregatedMetadata(VecPtr input, MPI_Comm mpiComm)
{
    if (input == nullptr)
    {
        Log(1,
            "DataManSerializer::PutAggregatedMetadata received nullptr input",
            true, true);
        return;
    }

    helper::BroadcastVector(*input, mpiComm);

    if (input->size() > 0)
    {
        nlohmann::json metaJ = DeserializeJson(input->data(), input->size());
        JsonToDataManVarMap(metaJ, nullptr);

        if (m_Verbosity >= 100)
        {
            std::cout << "DataManSerializer::PutAggregatedMetadata: "
                      << std::endl;
            std::cout << metaJ.dump(4) << std::endl;
        }
    }
}

/*
void DataManSerializer::AccumulateAggregatedMetadata(const VecPtr input,
                                                     VecPtr output)
{
    if (input->empty())
    {
        Log(1, "DataManSerializer::AccumulateAggregatedMetadata tries to "
               "deserialize "
               "an empty json pack",
            true, true);
    }

    nlohmann::json inputJ = DeserializeJson(input->data(), input->size());
    nlohmann::json outputJ;
    if (output->size() > 0)
    {
        outputJ = DeserializeJson(output->data(), output->size());
    }
    for (auto i = inputJ.begin(); i != inputJ.end(); ++i)
    {
        outputJ[i.key()] = i.value();
    }
    output = SerializeJson(outputJ);
}
*/

VecPtr DataManSerializer::EndSignal(size_t step)
{
    nlohmann::json j;
    j["FinalStep"] = step;
    std::string s = j.dump() + '\0';
    auto c = std::make_shared<std::vector<char>>(s.size());
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
        if (type == helper::GetType<int32_t>() ||
            type == helper::GetType<int64_t>() ||
            type == helper::GetType<float>() ||
            type == helper::GetType<double>())
        {
            return true;
        }
    }
    return false;
}

void DataManSerializer::PutAttributes(core::IO &io)
{
    const auto &attributesDataMap = io.GetAttributesDataMap();
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
        PutAttribute(attribute);                                               \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
}

void DataManSerializer::GetAttributes(core::IO &io)
{
    std::lock_guard<std::mutex> lStaticDataJson(m_StaticDataJsonMutex);
    for (const auto &staticVar : m_StaticDataJson["S"])
    {
        const std::string type(staticVar["Y"].get<std::string>());
        if (type == "")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        const auto &attributesDataMap = io.GetAttributesDataMap();             \
        auto it = attributesDataMap.find(staticVar["N"].get<std::string>());   \
        if (it == attributesDataMap.end())                                     \
        {                                                                      \
            if (staticVar["V"].get<bool>())                                    \
            {                                                                  \
                io.DefineAttribute<T>(staticVar["N"].get<std::string>(),       \
                                      staticVar["G"].get<T>());                \
            }                                                                  \
            else                                                               \
            {                                                                  \
                io.DefineAttribute<T>(                                         \
                    staticVar["N"].get<std::string>(),                         \
                    staticVar["G"].get<std::vector<T>>().data(),               \
                    staticVar["G"].get<std::vector<T>>().size());              \
            }                                                                  \
        }                                                                      \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
}

void DataManSerializer::AttachAttributes()
{
    std::lock_guard<std::mutex> l1(m_StaticDataJsonMutex);
    m_MetadataJson["S"] = m_StaticDataJson["S"];
}

void DataManSerializer::JsonToDataManVarMap(nlohmann::json &metaJ, VecPtr pack)
{

    // the mutex has to be locked here through the entire function. Otherwise
    // reader engine could get incomplete step metadata. This function only
    // deals with JSON metadata and data buffer already in allocated shared
    // pointers, so it should be cheap to lock.
    std::lock_guard<std::mutex> lDataManVarMapMutex(m_DataManVarMapMutex);

    for (auto stepMapIt = metaJ.begin(); stepMapIt != metaJ.end(); ++stepMapIt)
    {
        if (stepMapIt.key() == "S")
        {
            m_StaticDataJsonMutex.lock();
            m_StaticDataJson["S"] = stepMapIt.value();
            m_StaticDataJsonMutex.unlock();
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
                    var.type = varBlock["Y"].get<std::string>();
                    var.rank = stoi(rankMapIt.key());
                    var.address = varBlock["A"].get<std::string>();
                }
                catch (std::exception &e)
                {
                    throw(std::runtime_error(
                        "DataManSerializer::JsonToDataManVarMap missing "
                        "compulsory properties in JSON metadata"));
                }

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
                else
                {
                    var.isRowMajor = true;
                }

                itJson = varBlock.find("E");
                if (itJson != varBlock.end())
                {
                    var.isLittleEndian = itJson->get<bool>();
                }
                else
                {
                    var.isLittleEndian = true;
                }

                itJson = varBlock.find("S");
                if (itJson != varBlock.end())
                {
                    var.shape = itJson->get<Dims>();
                }

                itJson = varBlock.find("+");
                if (itJson != varBlock.end())
                {
                    var.max = itJson->get<std::vector<char>>();
                }

                itJson = varBlock.find("-");
                if (itJson != varBlock.end())
                {
                    var.min = itJson->get<std::vector<char>>();
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
        }
    }
    if (m_Verbosity >= 5)
    {
        std::cout
            << "DataManSerializer::JsonToDataManVarMap Total buffered steps = "
            << m_DataManVarMap.size() << ": ";
        for (const auto &i : m_DataManVarMap)
        {
            std::cout << i.first << ", ";
        }
        std::cout << std::endl;
    }
}

int DataManSerializer::PutPack(const VecPtr data)
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
    std::lock_guard<std::mutex> l2(m_AggregatedMetadataJsonMutex);
    if (allPreviousSteps)
    {
        std::vector<DmvVecPtrMap::iterator> its;
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
            Log(5,
                "DataManSerializer::Erase() trying to erase step " +
                    std::to_string(it->first) +
                    ". This is one of multiple steps being erased.",
                true, true);
            m_DataManVarMap.erase(it);
        }

        if (m_AggregatedMetadataJson != nullptr)
        {
            std::vector<nlohmann::json::iterator> jits;
            for (auto it = m_AggregatedMetadataJson.begin();
                 it != m_AggregatedMetadataJson.end(); ++it)
            {
                if (stoull(it.key()) < step)
                {
                    jits.push_back(it);
                }
            }
            for (auto it : jits)
            {
                m_AggregatedMetadataJson.erase(it);
            }
        }
    }
    else
    {
        Log(5,
            "DataManSerializer::Erase() trying to erase step " +
                std::to_string(step),
            true, true);
        m_DataManVarMap.erase(step);
        if (m_AggregatedMetadataJson != nullptr)
        {
            m_AggregatedMetadataJson.erase(std::to_string(step));
        }
    }
}

const DmvVecPtrMap DataManSerializer::GetMetaData()
{
    std::lock_guard<std::mutex> l(m_DataManVarMapMutex);
    return m_DataManVarMap;
}

DmvVecPtr DataManSerializer::GetMetaData(const size_t step)
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

int DataManSerializer::PutDeferredRequest(const std::string &variable,
                                          const size_t step, const Dims &start,
                                          const Dims &count, void *data)
{

    DmvVecPtr varVec;

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

            jmap[var.address].emplace_back();
            nlohmann::json &j = jmap[var.address].back();
            j["N"] = variable;
            j["O"] = var.start;
            j["C"] = var.count;
            j["T"] = step;
        }
    }

    for (const auto &i : jmap)
    {
        (*m_DeferredRequestsToSend)[i.first] = SerializeJson(i.second);
    }

    return 0;
}

DeferredRequestMapPtr DataManSerializer::GetDeferredRequest()
{
    auto t = m_DeferredRequestsToSend;
    m_DeferredRequestsToSend = std::make_shared<DeferredRequestMap>();
    return t;
}

VecPtr DataManSerializer::GenerateReply(const std::vector<char> &request,
                                        size_t &step)
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
        Log(1,
            "DataManSerializer::GenerateReply() received staging request "
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

        DmvVecPtr varVec;

        {
            std::lock_guard<std::mutex> l(m_DataManVarMapMutex);
            auto itVarVec = m_DataManVarMap.find(step);
            if (itVarVec == m_DataManVarMap.end())
            {
                Log(1,
                    "DataManSerializer::GenerateReply() received staging "
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
                    Log(1,
                        "DataManSerializer::GenerateReply() received "
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

                    auto metapack = SerializeJson(*replyMetaJ);
                    size_t metasize = metapack->size();
                    (reinterpret_cast<uint64_t *>(
                        replyLocalBuffer->data()))[0] =
                        replyLocalBuffer->size();
                    (reinterpret_cast<uint64_t *>(
                        replyLocalBuffer->data()))[1] = metasize;
                    replyLocalBuffer->resize(replyLocalBuffer->size() +
                                             metasize);
                    std::memcpy(replyLocalBuffer->data() +
                                    replyLocalBuffer->size() - metasize,
                                metapack->data(), metasize);
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

VecPtr DataManSerializer::SerializeJson(const nlohmann::json &message)
{
    auto pack = std::make_shared<std::vector<char>>();
    if (m_UseJsonSerialization == "msgpack")
    {
        nlohmann::json::to_msgpack(message, *pack);
    }
    else if (m_UseJsonSerialization == "cbor")
    {
        nlohmann::json::to_cbor(message, *pack);
    }
    else if (m_UseJsonSerialization == "ubjson")
    {
        nlohmann::json::to_ubjson(message, *pack);
    }
    else if (m_UseJsonSerialization == "string")
    {
        std::string pack_str = message.dump();
        pack->resize(pack_str.size() + 1);
        std::memcpy(pack->data(), pack_str.data(), pack_str.size());
        pack->back() = '\0';
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
    if (m_Verbosity >= 200)
    {
        std::cout << "DataManSerializer::DeserializeJson Json = ";
        for (size_t i = 0; i < size; ++i)
        {
            std::cout << start[i];
        }
        std::cout << std::endl;
        std::cout << size << std::endl;
    }

    if (start == nullptr or start == NULL or size == 0)
    {
        throw(std::runtime_error("DataManSerializer::DeserializeJson received "
                                 "uninitialized message"));
    }
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
