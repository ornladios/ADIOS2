/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscHelper.cpp
 *
 *  Created on: Sep 30, 2019
 *      Author: Jason Wang
 */

#include "SscHelper.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosJSONcomplex.h"
#include "adios2/helper/adiosType.h"
#include <iostream>
#include <numeric>

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

size_t GetTypeSize(DataType type)
{
    if (type == DataType::None)
    {
        throw(std::runtime_error("unknown data type"));
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>()) { return sizeof(T); }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    else { throw(std::runtime_error("unknown data type")); }
}

size_t TotalDataSize(const Dims &dims, DataType type, const ShapeID &shapeId)
{
    if (shapeId == ShapeID::GlobalArray || shapeId == ShapeID::LocalArray)
    {
        return std::accumulate(dims.begin(), dims.end(), GetTypeSize(type),
                               std::multiplies<size_t>());
    }
    else if (shapeId == ShapeID::GlobalValue || shapeId == ShapeID::LocalValue)
    {
        return GetTypeSize(type);
    }
    else
    {
        throw(std::runtime_error("ShapeID not supported"));
    }
    return 0;
}

size_t TotalDataSize(const BlockVec &bv)
{
    size_t s = 0;
    for (const auto &b : bv)
    {
        if (b.type == DataType::String)
        {
            s += b.bufferCount;
        }
        else
        {
            s += TotalDataSize(b.count, b.type, b.shapeId);
        }
    }
    return s;
}

RankPosMap CalculateOverlap(BlockVecVec &globalVecVec, const BlockVec &localVec)
{
    RankPosMap ret;
    int rank = 0;
    for (auto &rankBlockVec : globalVecVec)
    {
        for (auto &gBlock : rankBlockVec)
        {
            for (auto &lBlock : localVec)
            {
                if (lBlock.name == gBlock.name)
                {
                    if (gBlock.shapeId == ShapeID::GlobalValue)
                    {
                        ret[rank].first = 0;
                    }
                    else if (gBlock.shapeId == ShapeID::GlobalArray)
                    {
                        bool hasOverlap = true;
                        for (size_t i = 0; i < gBlock.start.size(); ++i)
                        {
                            if (gBlock.start[i] + gBlock.count[i] <=
                                    lBlock.start[i] or
                                lBlock.start[i] + lBlock.count[i] <=
                                    gBlock.start[i])
                            {
                                hasOverlap = false;
                                break;
                            }
                        }
                        if (hasOverlap)
                        {
                            ret[rank].first = 0;
                        }
                    }
                    else if (gBlock.shapeId == ShapeID::LocalValue)
                    {
                    }
                    else if (gBlock.shapeId == ShapeID::LocalArray)
                    {
                    }
                }
            }
        }
        ++rank;
    }
    return ret;
}

void BlockVecToJson(const BlockVec &input, nlohmann::json &output)
{
    for (const auto &b : input)
    {
        output["Variables"].emplace_back();
        auto &jref = output["Variables"].back();
        jref["Name"] = b.name;
        jref["Type"] = ToString(b.type);
        jref["ShapeID"] = b.shapeId;
        jref["Shape"] = b.shape;
        jref["Start"] = b.start;
        jref["Count"] = b.count;
        jref["BufferStart"] = b.bufferStart;
        jref["BufferCount"] = b.bufferCount;
        if (!b.value.empty())
        {
            jref["Value"] = b.value;
        }
    }
}

void AttributeMapToJson(IO &input, nlohmann::json &output)
{
    const auto &attributeMap = input.GetAttributesDataMap();
    auto &attributesJson = output["Attributes"];
    for (const auto &attributePair : attributeMap)
    {
        const std::string name(attributePair.first);
        const DataType type(attributePair.second.first);
        if (type == DataType::None)
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        const auto &attribute = input.InquireAttribute<T>(name);               \
        nlohmann::json attributeJson;                                          \
        attributeJson["Name"] = attribute->m_Name;                             \
        attributeJson["Type"] = ToString(attribute->m_Type);                   \
        attributeJson["IsSingleValue"] = attribute->m_IsSingleValue;           \
        if (attribute->m_IsSingleValue)                                        \
        {                                                                      \
            attributeJson["Value"] = attribute->m_DataSingleValue;             \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            attributeJson["Array"] = attribute->m_DataArray;                   \
        }                                                                      \
        output["Attributes"].emplace_back(std::move(attributeJson));           \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
}

void LocalJsonToGlobalJson(const std::vector<char> &input,
                           const size_t maxLocalSize, const int streamSize,
                           nlohmann::json &output)
{
    try
    {
        for (size_t i = 0; i < streamSize; ++i)
        {
            if (input[i * maxLocalSize] == '\0')
            {
                output[i] = nullptr;
            }
            else
            {
                output[i] = nlohmann::json::parse(
                    input.begin() + i * maxLocalSize,
                    input.begin() + (i + 1) * maxLocalSize);
            }
        }
    }
    catch (std::exception &e)
    {
        throw(std::runtime_error(
            std::string("corrupted global write pattern metadata, ") +
            std::string(e.what())));
    }
}

void JsonToBlockVecVec(const nlohmann::json &input, BlockVecVec &output)
{
    for (int i = 0; i < output.size(); ++i)
    {
        if (input[i] != nullptr)
        {
            auto &rankj = input[i]["Variables"];
            output[i].clear();
            for (const auto &j : rankj)
            {
                output[i].emplace_back();
                auto &b = output[i].back();
                b.name = j["Name"].get<std::string>();
                b.type =
                    helper::GetDataTypeFromString(j["Type"].get<std::string>());
                b.shapeId = j["ShapeID"].get<ShapeID>();
                b.start = j["Start"].get<Dims>();
                b.count = j["Count"].get<Dims>();
                b.shape = j["Shape"].get<Dims>();
                b.bufferStart = j["BufferStart"].get<size_t>();
                b.bufferCount = j["BufferCount"].get<size_t>();
                auto it = j.find("Value");
                if (it != j.end())
                {
                    auto value = it->get<std::vector<char>>();
                    b.value.resize(value.size());
                    std::memcpy(b.value.data(), value.data(), value.size());
                }
            }
        }
    }
}

void JsonToBlockVecVec(const std::string &input, BlockVecVec &output)
{
    nlohmann::json j;
    try
    {
        j = nlohmann::json::parse(input);
    }
    catch (...)
    {
        throw(std::runtime_error("corrupted json string"));
    }
    JsonToBlockVecVec(j, output);
}

void JsonToBlockVecVec(const std::vector<char> &input, BlockVecVec &output)
{
    nlohmann::json j;
    try
    {
        j = nlohmann::json::parse(input);
    }
    catch (...)
    {
        throw(std::runtime_error("corrupted json char vector"));
    }
    JsonToBlockVecVec(j, output);
}

bool AreSameDims(const Dims &a, const Dims &b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] != b[i])
        {
            return false;
        }
    }
    return true;
}

void PrintDims(const Dims &dims, const std::string &label)
{
    std::cout << label;
    for (const auto &i : dims)
    {
        std::cout << i << ", ";
    }
    std::cout << std::endl;
}

void PrintBlock(const BlockInfo &b, const std::string &label)
{
    std::cout << label << std::endl;
    std::cout << b.name << std::endl;
    std::cout << "    DataType : " << ToString(b.type) << std::endl;
    PrintDims(b.shape, "    Shape : ");
    PrintDims(b.start, "    Start : ");
    PrintDims(b.count, "    Count : ");
    std::cout << "    Position Start : " << b.bufferStart << std::endl;
    std::cout << "    Position Count : " << b.bufferCount << std::endl;
}

void PrintBlockVec(const BlockVec &bv, const std::string &label)
{
    std::cout << label << std::endl;
    for (const auto &i : bv)
    {
        std::cout << i.name << std::endl;
        std::cout << "    DataType : " << ToString(i.type) << std::endl;
        PrintDims(i.shape, "    Shape : ");
        PrintDims(i.start, "    Start : ");
        PrintDims(i.count, "    Count : ");
        std::cout << "    Position Start : " << i.bufferStart << std::endl;
        std::cout << "    Position Count : " << i.bufferCount << std::endl;
    }
}

void PrintBlockVecVec(const BlockVecVec &bvv, const std::string &label)
{
    std::cout << label << std::endl;
    size_t rank = 0;
    for (const auto &bv : bvv)
    {
        std::cout << "Rank " << rank << std::endl;
        for (const auto &i : bv)
        {
            std::cout << "    " << i.name << std::endl;
            std::cout << "        DataType : " << ToString(i.type) << std::endl;
            PrintDims(i.shape, "        Shape : ");
            PrintDims(i.start, "        Start : ");
            PrintDims(i.count, "        Count : ");
            std::cout << "        Position Start : " << i.bufferStart
                      << std::endl;
            std::cout << "        Position Count : " << i.bufferCount
                      << std::endl;
        }
        ++rank;
    }
}

void PrintRankPosMap(const RankPosMap &m, const std::string &label)
{
    std::cout << label << std::endl;
    for (const auto &rank : m)
    {
        std::cout << "Rank = " << rank.first
                  << ", bufferStart = " << rank.second.first
                  << ", bufferCount = " << rank.second.second << std::endl;
    }
}

void PrintMpiInfo(const MpiInfo &writersInfo, const MpiInfo &readersInfo)
{
    int s = 0;
    for (int i = 0; i < writersInfo.size(); ++i)
    {
        std::cout << "App " << s << " Writer App " << i << " Wrold Ranks : ";
        for (int j = 0; j < writersInfo[i].size(); ++j)
        {
            std::cout << writersInfo[i][j] << "  ";
        }
        std::cout << std::endl;
        ++s;
    }
    for (int i = 0; i < readersInfo.size(); ++i)
    {
        std::cout << "App " << s << " Reader App " << i << " Wrold Ranks : ";
        for (int j = 0; j < readersInfo[i].size(); ++j)
        {
            std::cout << readersInfo[i][j] << "  ";
        }
        std::cout << std::endl;
        ++s;
    }
    std::cout << std::endl;
}

} // end namespace ssc
} // end namespace engine
} // end namespace core
} // end namespace adios2
