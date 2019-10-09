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

#include <cstring>
#include <iostream>

namespace adios2
{
namespace format
{

DataManSerializer::DataManSerializer(helper::Comm const &comm,
                                     const bool isRowMajor)
: m_Comm(comm), m_IsRowMajor(isRowMajor),
  m_IsLittleEndian(helper::IsLittleEndian()),
  m_DeferredRequestsToSend(std::make_shared<DeferredRequestMap>())
{
    m_MpiRank = m_Comm.Rank();
    m_MpiSize = m_Comm.Size();
}

void DataManSerializer::NewWriterBuffer(size_t bufferSize)
{
    TAU_SCOPED_TIMER_FUNC();
    // make a new shared object each time because the old shared object could
    // still be alive and needed somewhere in the workflow, for example the
    // queue in transport manager. It will be automatically released when the
    // entire workflow finishes using it.
    m_MetadataJson = nullptr;
    m_LocalBuffer = std::make_shared<std::vector<char>>();
    m_LocalBuffer->reserve(bufferSize);
    m_LocalBuffer->resize(sizeof(uint64_t) * 2);
}

VecPtr DataManSerializer::GetLocalPack()
{
    TAU_SCOPED_TIMER_FUNC();
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

void DataManSerializer::AggregateMetadata()
{
    TAU_SCOPED_TIMER_FUNC();

    m_ProtectedStepsMutex.lock();
    for (const auto &idMap : m_ProtectedStepsToAggregate)
    {
        m_MetadataJson["P"][std::to_string(idMap.first)] = idMap.second;
    }
    m_ProtectedStepsMutex.unlock();

    auto localJsonPack = SerializeJson(m_MetadataJson);
    unsigned int size = localJsonPack->size();
    unsigned int maxSize;
    m_Comm.Allreduce(&size, &maxSize, 1, helper::Comm::Op::Max);
    maxSize += sizeof(uint64_t);
    localJsonPack->resize(maxSize, '\0');
    *(reinterpret_cast<uint64_t *>(localJsonPack->data() +
                                   localJsonPack->size()) -
      1) = size;

    std::vector<char> globalJsonStr(m_MpiSize * maxSize);
    m_Comm.Allgather(localJsonPack->data(), maxSize, globalJsonStr.data(),
                     maxSize);

    nlohmann::json aggMetadata;

    for (int i = 0; i < m_MpiSize; ++i)
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
            if (stepMapIt.key() == "P")
            {
                std::lock_guard<std::mutex> l(m_ProtectedStepsMutex);
                for (auto appidMapIt = stepMapIt.value().begin();
                     appidMapIt != stepMapIt.value().end(); ++appidMapIt)
                {
                    auto stepVecToAdd = appidMapIt->get<std::vector<size_t>>();
                    auto &stepVecExisted =
                        m_ProtectedStepsAggregated[stoull(appidMapIt.key())];
                    for (const auto &protectedStep : stepVecToAdd)
                    {
                        auto it =
                            std::find(stepVecExisted.begin(),
                                      stepVecExisted.end(), protectedStep);
                        if (it == stepVecExisted.end())
                        {
                            stepVecExisted.push_back(protectedStep);
                        }
                    }
                    std::sort(stepVecExisted.begin(), stepVecExisted.end());
                }
                if (m_Verbosity >= 5)
                {
                    std::cout << "Rank ";
                    std::cout << m_MpiRank;
                    std::cout << " All protected steps aggregated before "
                                 "reducing are: ";
                    for (const auto &stepVecPair : m_ProtectedStepsAggregated)
                    {
                        for (const auto &step : stepVecPair.second)
                        {
                            std::cout << step << ", ";
                        }
                    }
                    std::cout << std::endl;
                }
                for (auto &stepVecPair : m_ProtectedStepsAggregated)
                {
                    while (stepVecPair.second.size() > 3)
                    {
                        stepVecPair.second.erase(stepVecPair.second.begin());
                    }
                }
                if (m_Verbosity >= 5)
                {
                    std::cout << "Rank ";
                    std::cout << m_MpiRank;
                    std::cout << " All protected steps aggregated after "
                                 "reducing are: ";
                    for (const auto &stepVecPair : m_ProtectedStepsAggregated)
                    {
                        for (const auto &step : stepVecPair.second)
                        {
                            std::cout << step << ", ";
                        }
                    }
                    std::cout << std::endl;
                }
            }
            else
            {
                for (auto rankMapIt = stepMapIt.value().begin();
                     rankMapIt != stepMapIt.value().end(); ++rankMapIt)
                {
                    aggMetadata[stepMapIt.key()][rankMapIt.key()] =
                        rankMapIt.value();
                }
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

VecPtr DataManSerializer::GetAggregatedMetadataPack(const int64_t stepRequested,
                                                    int64_t &stepProvided,
                                                    const int64_t appID)
{

    TAU_SCOPED_TIMER_FUNC();

    std::lock_guard<std::mutex> l(m_AggregatedMetadataJsonMutex);

    VecPtr ret = nullptr;

    stepProvided = -1;

    if (stepRequested == -1) // getting the earliest step
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
        if (min < std::numeric_limits<int64_t>::max())
        {
            nlohmann::json retJ;
            retJ[std::to_string(min)] =
                m_AggregatedMetadataJson[std::to_string(min)];
            ret = SerializeJson(retJ);
            stepProvided = min;
        }
    }
    else if (stepRequested == -2) // getting the latest step
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
        if (max >= 0)
        {
            nlohmann::json retJ;
            retJ[std::to_string(max)] =
                m_AggregatedMetadataJson[std::to_string(max)];
            ret = SerializeJson(retJ);
            stepProvided = max;
        }
    }
    else if (stepRequested == -3) // getting static variables
    {
        ret = SerializeJson(m_StaticDataJson);
    }
    else if (stepRequested == -4) // getting all steps
    {
        ret = SerializeJson(m_AggregatedMetadataJson);
    }
    else
    {
        auto it = m_AggregatedMetadataJson.find(std::to_string(stepRequested));
        if (it != m_AggregatedMetadataJson.end())
        {
            nlohmann::json retJ;
            retJ[std::to_string(stepRequested)] = *it;
            ret = SerializeJson(retJ);
            stepProvided = stepRequested;
        }
    }

    if (stepProvided > -1 and appID > -1)
    {
        ProtectStep(stepProvided, appID);
    }

    return ret;
}

void DataManSerializer::PutAggregatedMetadata(VecPtr input,
                                              helper::Comm const &comm)
{
    TAU_SCOPED_TIMER_FUNC();
    if (input == nullptr)
    {
        Log(1,
            "DataManSerializer::PutAggregatedMetadata received nullptr input",
            true, true);
        return;
    }

    comm.BroadcastVector(*input);

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

bool DataManSerializer::IsCompressionAvailable(const std::string &method,
                                               const std::string &type,
                                               const Dims &count)
{
    TAU_SCOPED_TIMER_FUNC();
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
    TAU_SCOPED_TIMER_FUNC();
    const auto &attributesDataMap = io.GetAttributesDataMap();
    bool attributePut = false;
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
        attributePut = true;
    }

    if (not m_StaticDataFinished)
    {
        if (not attributePut)
        {
            nlohmann::json staticVar;
            staticVar["N"] = "NoAttributes";
            staticVar["Y"] = "bool";
            staticVar["V"] = true;
            staticVar["G"] = true;
            m_StaticDataJsonMutex.lock();
            m_StaticDataJson["S"].emplace_back(std::move(staticVar));
            m_StaticDataJsonMutex.unlock();
        }
        m_StaticDataFinished = true;
    }
}

void DataManSerializer::GetAttributes(core::IO &io)
{
    TAU_SCOPED_TIMER_FUNC();
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
    TAU_SCOPED_TIMER_FUNC();
    std::lock_guard<std::mutex> l1(m_StaticDataJsonMutex);
    m_MetadataJson["S"] = m_StaticDataJson["S"];
}

void DataManSerializer::JsonToDataManVarMap(nlohmann::json &metaJ, VecPtr pack)
{
    TAU_SCOPED_TIMER_FUNC();

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

        size_t step = stoull(stepMapIt.key());
        m_DeserializedBlocksForStepMutex.lock();
        auto blocksForStepIt = m_DeserializedBlocksForStep.find(step);
        if (blocksForStepIt == m_DeserializedBlocksForStep.end())
        {
            m_DeserializedBlocksForStep[step] = 1;
        }
        else
        {
            ++m_DeserializedBlocksForStep[step];
        }
        m_DeserializedBlocksForStepMutex.unlock();

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
                }
                catch (std::exception &e)
                {
                    throw(std::runtime_error(
                        "DataManSerializer::JsonToDataManVarMap missing "
                        "compulsory properties in JSON metadata"));
                }

                // optional properties

                auto itJson = varBlock.find("A");
                if (itJson != varBlock.end())
                {
                    var.address = itJson->get<std::string>();
                }

                itJson = varBlock.find("D");
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
    TAU_SCOPED_TIMER_FUNC();
    if (data->size() == 0)
    {
        return -1;
    }
    uint64_t metaPosition =
        (reinterpret_cast<const uint64_t *>(data->data()))[0];
    uint64_t metaSize = (reinterpret_cast<const uint64_t *>(data->data()))[1];
    nlohmann::json j = DeserializeJson(data->data() + metaPosition, metaSize);
    JsonToDataManVarMap(j, data);
    return 0;
}

void DataManSerializer::Erase(const size_t step, const bool allPreviousSteps)
{
    TAU_SCOPED_TIMER_FUNC();
    std::lock_guard<std::mutex> l1(m_DataManVarMapMutex);
    std::lock_guard<std::mutex> l2(m_AggregatedMetadataJsonMutex);
    if (allPreviousSteps)
    {
        std::vector<DmvVecPtrMap::iterator> its;
        for (auto it = m_DataManVarMap.begin(); it != m_DataManVarMap.end();
             ++it)
        {
            if (it->first <= step)
            {
                if (not IsStepProtected(it->first))
                {
                    its.push_back(it);
                }
                else
                {
                    Log(5,
                        "DataManSerializer::Erase() trying to erase step " +
                            std::to_string(it->first) + ", but it is protected",
                        true, true);
                }
            }
        }
        for (auto it : its)
        {
            m_DataManVarMap.erase(it);
            Log(5,
                "DataManSerializer::Erase() erased step " +
                    std::to_string(it->first),
                true, true);
        }
        if (m_AggregatedMetadataJson != nullptr)
        {
            std::vector<nlohmann::json::iterator> jits;
            for (auto it = m_AggregatedMetadataJson.begin();
                 it != m_AggregatedMetadataJson.end(); ++it)
            {
                if (stoull(it.key()) < step)
                {
                    if (not IsStepProtected(stoull(it.key())))
                    {
                        jits.push_back(it);
                    }
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
        if (not IsStepProtected(step))
        {
            m_DataManVarMap.erase(step);
            if (m_AggregatedMetadataJson != nullptr)
            {
                m_AggregatedMetadataJson.erase(std::to_string(step));
            }
            Log(5,
                "DataManSerializer::Erase() erased step " +
                    std::to_string(step),
                true, true);
        }
        else
        {
            Log(5,
                "DataManSerializer::Erase() trying to erase step " +
                    std::to_string(step) + ", but it is protected",
                true, true);
        }
    }
}

DmvVecPtrMap DataManSerializer::GetFullMetadataMap()
{
    TAU_SCOPED_TIMER_FUNC();
    std::lock_guard<std::mutex> l(m_DataManVarMapMutex);
    return m_DataManVarMap;
}

DmvVecPtr DataManSerializer::GetStepMetadata(const size_t step)
{
    TAU_SCOPED_TIMER_FUNC();
    std::lock_guard<std::mutex> l(m_DataManVarMapMutex);
    auto it = m_DataManVarMap.find(step);
    if (it != m_DataManVarMap.end())
    {
        return it->second;
    }
    return nullptr;
}

int DataManSerializer::PutDeferredRequest(const std::string &variable,
                                          const size_t step, const Dims &start,
                                          const Dims &count, void *data)
{

    TAU_SCOPED_TIMER_FUNC();
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
                throw("DataManSerializer::PutDeferredRequest() requested "
                      "start, count and shape do not match");
                continue;
            }
            bool toContinue = false;
            for (size_t i = 0; i < start.size(); ++i)
            {
                if (start[i] >= var.start[i] + var.count[i] ||
                    start[i] + count[i] <= var.start[i])
                {
                    toContinue = true;
                }
            }
            if (toContinue)
            {
                continue;
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
        auto charVec = (*m_DeferredRequestsToSend)[i.first];
        if (charVec == nullptr)
        {
            charVec = std::make_shared<std::vector<char>>();
        }
        nlohmann::json jsonSer;
        if (charVec->size() > 0)
        {
            jsonSer = DeserializeJson(charVec->data(), charVec->size());
        }
        for (auto j = i.second.begin(); j != i.second.end(); ++j)
        {
            jsonSer.push_back(*j);
        }
        (*m_DeferredRequestsToSend)[i.first] = SerializeJson(jsonSer);
    }

    return 0;
}

DeferredRequestMapPtr DataManSerializer::GetDeferredRequest()
{
    TAU_SCOPED_TIMER_FUNC();
    auto t = m_DeferredRequestsToSend;
    m_DeferredRequestsToSend = std::make_shared<DeferredRequestMap>();
    return t;
}

VecPtr DataManSerializer::GenerateReply(
    const std::vector<char> &request, size_t &step,
    const std::unordered_map<std::string, Params> &compressionParams)
{
    TAU_SCOPED_TIMER_FUNC();
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
                Params compressionParamsVar;
                auto compressionParamsIter = compressionParams.find(var.name);
                if (compressionParamsIter != compressionParams.end())
                {
                    compressionParamsVar = compressionParamsIter->second;
                }
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
               var.rank, var.address, compressionParamsVar, replyLocalBuffer,  \
               replyMetaJ);                                                    \
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
    TAU_SCOPED_TIMER_FUNC();

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
        if (inStart[i] + inCount[i] <= outStart[i])
        {
            return false;
        }
        if (outStart[i] + outCount[i] <= inStart[i])
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

/*
size_t DataManSerializer::MinStep()
{
    TAU_SCOPED_TIMER_FUNC();
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
*/

size_t DataManSerializer::LocalBufferSize() { return m_LocalBuffer->size(); }

VecPtr DataManSerializer::SerializeJson(const nlohmann::json &message)
{
    TAU_SCOPED_TIMER_FUNC();
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
    TAU_SCOPED_TIMER_FUNC();
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

void DataManSerializer::ProtectStep(const int64_t step, const int64_t id)
{
    TAU_SCOPED_TIMER_FUNC();
    std::lock_guard<std::mutex> l(m_ProtectedStepsMutex);
    m_ProtectedStepsToAggregate[id].push_back(step);
    auto &idVec = m_ProtectedStepsToAggregate[id];
    while (idVec.size() > 3)
    {
        idVec.erase(idVec.begin());
    }
    if (m_Verbosity >= 5)
    {
        std::cout << "Rank ";
        std::cout << m_MpiRank;
        std::cout << " Step ";
        std::cout << step;
        std::cout << " is protected for App ";
        std::cout << id;
        std::cout << ". All protected steps to be aggregated are: ";
        for (auto i : m_ProtectedStepsToAggregate[id])
        {
            std::cout << i << ", ";
        }
        std::cout << std::endl;
    }
}

bool DataManSerializer::IsStepProtected(const int64_t step)
{
    TAU_SCOPED_TIMER_FUNC();
    std::lock_guard<std::mutex> l(m_ProtectedStepsMutex);
    bool ret = false;
    for (const auto &stepVecPair : m_ProtectedStepsAggregated)
    {
        for (const auto &stepProtected : stepVecPair.second)
            if (stepProtected == step)
            {
                ret = true;
            }
    }
    return ret;
}

void DataManSerializer::SetDestination(const std::string &dest)
{
    m_Destination = dest;
}

std::string DataManSerializer::GetDestination() { return m_Destination; }

bool DataManSerializer::StepHasMinimumBlocks(const size_t step,
                                             const int requireMinimumBlocks)
{
    std::lock_guard<std::mutex> l(m_DeserializedBlocksForStepMutex);
    auto it = m_DeserializedBlocksForStep.find(step);
    if (it != m_DeserializedBlocksForStep.end())
    {
        if (it->second >= requireMinimumBlocks)
        {
            return true;
        }
    }
    return false;
}

DmvVecPtr DataManSerializer::GetEarliestLatestStep(
    int64_t &currentStep, const int requireMinimumBlocks,
    const float timeoutSeconds, const bool latest)
{
    TAU_SCOPED_TIMER_FUNC();

    auto start_time = std::chrono::system_clock::now();
    while (true)
    {
        std::lock_guard<std::mutex> l(m_DataManVarMapMutex);

        bool hasStep = false;
        size_t latestStep = 0;
        size_t earliestStep = std::numeric_limits<size_t>::max();

        for (const auto &i : m_DataManVarMap)
        {
            if (latestStep < i.first)
            {
                latestStep = i.first;
            }
            if (earliestStep > i.first)
            {
                earliestStep = i.first;
            }
            hasStep = true;
        }

        if (hasStep)
        {
            bool hasCompleteStep = false;
            if (latest)
            {
                for (size_t step = latestStep; step >= earliestStep; --step)
                {
                    if (StepHasMinimumBlocks(step, requireMinimumBlocks))
                    {
                        currentStep = step;
                        hasCompleteStep = true;
                        break;
                    }
                }
            }
            else
            {
                for (size_t step = earliestStep; step <= latestStep; ++step)
                {
                    if (StepHasMinimumBlocks(step, requireMinimumBlocks))
                    {
                        currentStep = step;
                        hasCompleteStep = true;
                        break;
                    }
                }
            }

            if (hasCompleteStep)
            {
                return m_DataManVarMap[currentStep];
            }
        }

        auto now_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now_time - start_time);
        if (duration.count() > timeoutSeconds and timeoutSeconds > 0)
        {
            return nullptr;
        }
    }
    return nullptr;
}

void DataManSerializer::Log(const int level, const std::string &message,
                            const bool mpi, const bool endline)
{
    TAU_SCOPED_TIMER_FUNC();
    const int rank = m_Comm.World().Rank();

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
