/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Deserializer.tcc
 *
 *  Created on: Sep 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_TCC_

#include "BP3Deserializer.h"

#include <algorithm> //std::reverse
#include <unordered_set>

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
void BP3Deserializer::GetSyncVariableDataFromStream(core::Variable<T> &variable,
                                                    BufferSTL &bufferSTL) const
{
    auto itStep = variable.m_AvailableStepBlockIndexOffsets.find(
        variable.m_StepsStart + 1);

    if (itStep == variable.m_AvailableStepBlockIndexOffsets.end())
    {
        variable.m_Data = nullptr;
        return;
    }

    auto &buffer = bufferSTL.m_Buffer;
    size_t position = itStep->second.front();

    const Characteristics<T> characteristics =
        ReadElementIndexCharacteristics<T>(
            buffer, position, static_cast<DataTypes>(GetDataType<T>()), false);

    const size_t payloadOffset = characteristics.Statistics.PayloadOffset;
    variable.m_Data = reinterpret_cast<T *>(&buffer[payloadOffset]);
}

template <class T>
typename core::Variable<T>::Info &
BP3Deserializer::InitVariableBlockInfo(core::Variable<T> &variable, T *data)
{
    const size_t stepsStart = variable.m_StepsStart;
    const auto &indices = variable.m_AvailableStepBlockIndexOffsets;
    // bp3 starts at step "1"
    auto itStep = indices.find(stepsStart + 1);
    if (m_DebugMode)
    {
        if (itStep == indices.end())
        {
            throw std::invalid_argument("ERROR: step start in Variable " +
                                        variable.m_Name +
                                        " is invalid, in call to Get\n");
        }
    }
    const size_t stepsCount = variable.m_StepsCount;
    // create block info
    return variable.SetBlockInfo(data, stepsStart, stepsCount);
}

template <class T>
void BP3Deserializer::SetVariableBlockInfo(
    core::Variable<T> &variable, typename core::Variable<T>::Info &blockInfo)
{
    auto lf_SetSubStreamInfoOperations =
        [&](const BP3OpInfo &bp3OpInfo, const size_t payloadOffset,
            helper::SubStreamBoxInfo &subStreamInfo, const bool isRowMajor)

    {
        helper::BlockOperationInfo blockOperation;
        blockOperation.PayloadOffset = payloadOffset;
        blockOperation.PreShape = bp3OpInfo.PreShape;
        blockOperation.PreStart = bp3OpInfo.PreStart;
        blockOperation.PreCount = bp3OpInfo.PreCount;
        blockOperation.Info["PreDataType"] = helper::GetType<T>();
        // TODO: need to verify it's a match with PreDataType
        // std::to_string(static_cast<size_t>(bp3OpInfo.PreDataType));
        blockOperation.Info["Type"] = bp3OpInfo.Type;
        blockOperation.PreSizeOf = sizeof(T);

        // read metadata from supported type and populate Info
        std::shared_ptr<BP3Operation> bpOp = SetBP3Operation(bp3OpInfo.Type);
        bpOp->GetMetadata(bp3OpInfo.Metadata, blockOperation.Info);
        blockOperation.PayloadSize = static_cast<size_t>(
            std::stoull(blockOperation.Info.at("OutputSize")));

        subStreamInfo.OperationsInfo.push_back(std::move(blockOperation));
    };

    auto lf_SetSubStreamInfo =
        [&](const std::string &variableName, const Box<Dims> &selectionBox,
            typename core::Variable<T>::Info &blockInfo, const size_t step,
            const std::vector<size_t> &blockIndexOffsets,
            const BufferSTL &bufferSTL, const bool isRowMajor)

    {
        const std::vector<char> &buffer = bufferSTL.m_Buffer;

        for (const size_t blockIndexOffset : blockIndexOffsets)
        {
            size_t position = blockIndexOffset;

            const Characteristics<T> blockCharacteristics =
                ReadElementIndexCharacteristics<T>(
                    buffer, position, static_cast<DataTypes>(GetDataType<T>()));

            // check if they intersect
            helper::SubStreamBoxInfo subStreamInfo;

            if (helper::GetTotalSize(blockCharacteristics.Count) == 0)
            {
                subStreamInfo.ZeroBlock = true;
            }

            subStreamInfo.BlockBox = helper::StartEndBox(
                blockCharacteristics.Start, blockCharacteristics.Count);
            subStreamInfo.IntersectionBox =
                helper::IntersectionBox(selectionBox, subStreamInfo.BlockBox);

            if (subStreamInfo.IntersectionBox.first.empty() ||
                subStreamInfo.IntersectionBox.second.empty())
            {
                continue;
            }

            if (m_DebugMode)
            {
                const size_t dimensions = blockCharacteristics.Shape.size();
                if (dimensions != blockInfo.Shape.size())
                {
                    throw std::invalid_argument(
                        "ERROR: block Shape (available) and "
                        "selection Shape (requested) dimensions, do not match "
                        "when reading variable " +
                        variableName + ", in call to Get");
                }

                Dims readInShape = blockCharacteristics.Shape;
                if (m_ReverseDimensions)
                {
                    std::reverse(readInShape.begin(), readInShape.end());
                }

                for (auto i = 0; i < dimensions; ++i)
                {

                    if (blockInfo.Start[i] + blockInfo.Count[i] >
                        readInShape[i])
                    {
                        throw std::invalid_argument(
                            "ERROR: selection Start " +
                            helper::DimsToString(blockInfo.Start) +
                            " and Count " +
                            helper::DimsToString(blockInfo.Count) +
                            " (requested) is out of bounds of available "
                            "Shape " +
                            helper::DimsToString(blockCharacteristics.Shape) +
                            " , when reading variable " + variableName +
                            ", in call to Get");
                    }
                }
            }

            // relative position
            subStreamInfo.Seeks.first =
                sizeof(T) *
                helper::LinearIndex(subStreamInfo.BlockBox,
                                    subStreamInfo.IntersectionBox.first,
                                    isRowMajor);

            subStreamInfo.Seeks.second =
                sizeof(T) *
                (helper::LinearIndex(subStreamInfo.BlockBox,
                                     subStreamInfo.IntersectionBox.second,
                                     isRowMajor) +
                 1);

            const size_t payloadOffset =
                blockCharacteristics.Statistics.PayloadOffset;
            const auto &bp3Op = blockCharacteristics.Statistics.Op;
            // if they intersect get info Seeks (first: start, second:
            // count) depending on operation info
            if (bp3Op.IsActive)
            {
                lf_SetSubStreamInfoOperations(bp3Op, payloadOffset,
                                              subStreamInfo, m_IsRowMajor);
            }
            else
            {
                // make it absolute if no operations
                subStreamInfo.Seeks.first += payloadOffset;
                subStreamInfo.Seeks.second += payloadOffset;
            }
            subStreamInfo.SubStreamID =
                static_cast<size_t>(blockCharacteristics.Statistics.FileIndex);

            blockInfo.StepBlockSubStreamsInfo[step].push_back(
                std::move(subStreamInfo));
        }
    };

    const auto &indices = variable.m_AvailableStepBlockIndexOffsets;

    const Box<Dims> selectionBox = helper::StartEndBox(
        blockInfo.Start, blockInfo.Count, m_ReverseDimensions);

    auto itStep = indices.find(blockInfo.StepsStart + 1);

    for (auto i = 0; i < blockInfo.StepsCount; ++i)
    {
        if (m_DebugMode)
        {
            if (itStep == indices.end())
            {
                throw std::invalid_argument(
                    "ERROR: offset " + std::to_string(i) +
                    " from stepsStart in variable " + variable.m_Name +
                    " is beyond the largest available step =" +
                    std::to_string(indices.rbegin()->first) +
                    ", check Variable SetStepSelection argument stepsCount,"
                    "in call to Get");
            }
        }

        lf_SetSubStreamInfo(variable.m_Name, selectionBox, blockInfo,
                            itStep->first, itStep->second, m_Metadata,
                            m_IsRowMajor);
        ++itStep;
    }
}

template <class T>
void BP3Deserializer::GetValueFromMetadata(core::Variable<T> &variable,
                                           T *data) const
{
    GetValueFromMetadataCommon(variable, data);
}

template <class T>
bool BP3Deserializer::IdentityOperation(
    const std::vector<typename core::Variable<T>::Operation> &operations) const
    noexcept
{
    bool identity = false;
    for (const typename core::Variable<T>::Operation &op : operations)
    {
        if (op.Op->m_Type == "identity")
        {
            identity = true;
        }
    }
    return identity;
}

template <class T>
std::map<size_t, std::vector<typename core::Variable<T>::Info>>
BP3Deserializer::AllStepsBlocksInfo(const core::Variable<T> &variable) const
{
    std::map<size_t, std::vector<typename core::Variable<T>::Info>>
        allStepsBlocksInfo;

    for (const auto &pair : variable.m_AvailableStepBlockIndexOffsets)
    {
        const size_t step = pair.first;
        const std::vector<size_t> &blockPositions = pair.second;
        // bp3 index starts at 1
        allStepsBlocksInfo[step - 1] =
            BlocksInfoCommon(variable, blockPositions);
    }
    return allStepsBlocksInfo;
}

template <class T>
std::vector<typename core::Variable<T>::Info>
BP3Deserializer::BlocksInfo(const core::Variable<T> &variable,
                            const size_t step) const
{
    // bp3 format starts at 1
    auto itStep = variable.m_AvailableStepBlockIndexOffsets.find(step + 1);
    if (itStep == variable.m_AvailableStepBlockIndexOffsets.end())
    {
        return std::vector<typename core::Variable<T>::Info>();
    }
    return BlocksInfoCommon(variable, itStep->second);
}

template <class T>
void BP3Deserializer::ClipContiguousMemory(
    typename core::Variable<T>::Info &blockInfo,
    const std::vector<char> &contiguousMemory, const Box<Dims> &blockBox,
    const Box<Dims> &intersectionBox) const
{
    helper::ClipContiguousMemory(
        blockInfo.Data, blockInfo.Start, blockInfo.Count, contiguousMemory,
        blockBox, intersectionBox, m_IsRowMajor, m_ReverseDimensions);
}

// PRIVATE
template <>
inline void BP3Deserializer::DefineVariableInIO<std::string>(
    const ElementIndexHeader &header, core::IO &io,
    const std::vector<char> &buffer, size_t position) const
{
    const size_t initialPosition = position;

    const Characteristics<std::string> characteristics =
        ReadElementIndexCharacteristics<std::string>(
            buffer, position, static_cast<DataTypes>(header.DataType));

    std::string variableName(header.Name);
    if (!header.Path.empty())
    {
        variableName = header.Path + PathSeparator + header.Name;
    }

    core::Variable<std::string> *variable = nullptr;
    if (characteristics.Statistics.IsValue)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        variable = &io.DefineVariable<std::string>(variableName);
        variable->m_Value =
            characteristics.Statistics.Value; // assigning first step
    }
    else
    {
        throw std::runtime_error("ERROR: variable " + variableName +
                                 " of type string can't be an array, when "
                                 "parsing metadata in call to Open");
    }

    // going back to get variable index position
    variable->m_IndexStart =
        initialPosition - (header.Name.size() + header.GroupName.size() +
                           header.Path.size() + 23);

    const size_t endPosition =
        variable->m_IndexStart + static_cast<size_t>(header.Length) + 4;

    position = initialPosition;

    size_t currentStep = 0; // Starts at 1 in bp file
    std::unordered_set<uint32_t> stepsFound;
    variable->m_AvailableStepsCount = 0;
    while (position < endPosition)
    {
        const size_t subsetPosition = position;

        // read until step is found
        const Characteristics<std::string> subsetCharacteristics =
            ReadElementIndexCharacteristics<std::string>(
                buffer, position, static_cast<DataTypes>(header.DataType),
                false);

        if (subsetCharacteristics.Statistics.Step > currentStep)
        {
            currentStep = subsetCharacteristics.Statistics.Step;
        }
        if (stepsFound.insert(subsetCharacteristics.Statistics.Step).second)
        {
            ++variable->m_AvailableStepsCount;
        }
        variable->m_AvailableStepBlockIndexOffsets[currentStep].push_back(
            subsetPosition);
        position = subsetPosition + subsetCharacteristics.EntryLength + 5;
    }
    /* Update variable's starting step, which equals to the min value in the
     * sorted map minus one */
    variable->m_StepsStart =
        variable->m_AvailableStepBlockIndexOffsets.begin()->first - 1;
}

template <class T>
void BP3Deserializer::DefineVariableInIO(const ElementIndexHeader &header,
                                         core::IO &io,
                                         const std::vector<char> &buffer,
                                         size_t position) const
{
    const size_t initialPosition = position;

    Characteristics<T> characteristics = ReadElementIndexCharacteristics<T>(
        buffer, position, static_cast<DataTypes>(header.DataType));

    std::string variableName(header.Name);
    if (!header.Path.empty())
    {
        variableName = header.Path + PathSeparator + header.Name;
    }

    core::Variable<T> *variable = nullptr;
    if (characteristics.Statistics.IsValue)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        variable = &io.DefineVariable<T>(variableName);
        variable->m_Value = characteristics.Statistics.Value;
        variable->m_Min = characteristics.Statistics.Value;
        variable->m_Max = characteristics.Statistics.Value;
    }
    else
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_ReverseDimensions)
        {
            std::reverse(characteristics.Shape.begin(),
                         characteristics.Shape.end());
        }

        variable = &io.DefineVariable<T>(variableName, characteristics.Shape,
                                         Dims(characteristics.Shape.size(), 0),
                                         characteristics.Shape);

        variable->m_Min = characteristics.Statistics.Min;
        variable->m_Max = characteristics.Statistics.Max;
    }

    // going back to get variable index position
    variable->m_IndexStart =
        initialPosition - (header.Name.size() + header.GroupName.size() +
                           header.Path.size() + 23);

    const size_t endPosition =
        variable->m_IndexStart + static_cast<size_t>(header.Length) + 4;

    position = initialPosition;

    size_t currentStep = 0; // Starts at 1 in bp file
    std::unordered_set<uint32_t> stepsFound;
    variable->m_AvailableStepsCount = 0;
    while (position < endPosition)
    {
        const size_t subsetPosition = position;

        // read until step is found
        const Characteristics<T> subsetCharacteristics =
            ReadElementIndexCharacteristics<T>(
                buffer, position, static_cast<DataTypes>(header.DataType),
                false);

        if (helper::LessThan(subsetCharacteristics.Statistics.Min,
                             variable->m_Min))
        {
            variable->m_Min = subsetCharacteristics.Statistics.Min;
        }

        if (helper::GreaterThan(subsetCharacteristics.Statistics.Max,
                                variable->m_Max))
        {
            variable->m_Max = subsetCharacteristics.Statistics.Max;
        }

        if (subsetCharacteristics.Statistics.Step > currentStep)
        {
            currentStep = subsetCharacteristics.Statistics.Step;
        }
        if (stepsFound.insert(subsetCharacteristics.Statistics.Step).second)
        {
            ++variable->m_AvailableStepsCount;
        }
        variable->m_AvailableStepBlockIndexOffsets[currentStep].push_back(
            subsetPosition);
        position = subsetPosition + subsetCharacteristics.EntryLength + 5;
    }
    /* Update variable's starting step, which equals to the min value in the
     * sorted map minus one */
    variable->m_StepsStart =
        variable->m_AvailableStepBlockIndexOffsets.begin()->first - 1;
}

template <class T>
void BP3Deserializer::DefineAttributeInIO(const ElementIndexHeader &header,
                                          core::IO &io,
                                          const std::vector<char> &buffer,
                                          size_t position) const
{
    const Characteristics<T> characteristics =
        ReadElementIndexCharacteristics<T>(
            buffer, position, static_cast<DataTypes>(header.DataType));

    std::string attributeName(header.Name);
    if (!header.Path.empty())
    {
        attributeName = header.Path + PathSeparator + header.Name;
    }

    if (characteristics.Statistics.IsValue)
    {
        io.DefineAttribute<T>(attributeName, characteristics.Statistics.Value);
    }
    else
    {
        io.DefineAttribute<T>(attributeName,
                              characteristics.Statistics.Values.data(),
                              characteristics.Statistics.Values.size());
    }
}

template <>
inline void BP3Deserializer::GetValueFromMetadataCommon<std::string>(
    core::Variable<std::string> &variable, std::string *data) const
{
    const auto &buffer = m_Metadata.m_Buffer;

    for (size_t i = 0; i < variable.m_StepsCount; ++i)
    {
        *(data + i) = "";
        const size_t step = variable.m_StepsStart + i + 1;
        auto itStep = variable.m_AvailableStepBlockIndexOffsets.find(step);

        if (itStep == variable.m_AvailableStepBlockIndexOffsets.end())
        {
            continue;
        }

        for (const size_t position : itStep->second)
        {
            size_t localPosition = position;
            const Characteristics<std::string> characteristics =
                ReadElementIndexCharacteristics<std::string>(
                    buffer, localPosition, type_string, false);

            *(data + i) = characteristics.Statistics.Value;
        }

        variable.m_Value = *(data + i);
    }
}

template <class T>
inline void
BP3Deserializer::GetValueFromMetadataCommon(core::Variable<T> &variable,
                                            T *data) const
{
    const auto &buffer = m_Metadata.m_Buffer;
    const size_t step = variable.m_StepsStart + 1;
    auto itStep = variable.m_AvailableStepBlockIndexOffsets.find(step);

    if (itStep == variable.m_AvailableStepBlockIndexOffsets.end())
    {
        data = nullptr;
        return;
    }

    for (const size_t position : itStep->second)
    {
        size_t localPosition = position;

        const Characteristics<T> characteristics =
            ReadElementIndexCharacteristics<T>(
                buffer, localPosition, static_cast<DataTypes>(GetDataType<T>()),
                false);

        *data = characteristics.Statistics.Value;
    }
}

template <class T>
std::map<std::string, helper::SubFileInfoMap>
BP3Deserializer::GetSyncVariableSubFileInfo(
    const core::Variable<T> &variable) const
{
    std::map<std::string, helper::SubFileInfoMap> variableSubFileInfo;
    variableSubFileInfo[variable.m_Name] = GetSubFileInfo(variable);
    return variableSubFileInfo;
}

template <class T>
void BP3Deserializer::GetDeferredVariable(core::Variable<T> &variable, T *data)
{
    variable.m_Data = data;
    m_DeferredVariablesMap[variable.m_Name] = helper::SubFileInfoMap();
}

template <class T>
helper::SubFileInfoMap
BP3Deserializer::GetSubFileInfo(const core::Variable<T> &variable) const
{
    helper::SubFileInfoMap infoMap;

    const auto &buffer = m_Metadata.m_Buffer;

    const size_t stepStart = variable.m_StepsStart + 1;
    const size_t stepEnd = stepStart + variable.m_StepsCount; // exclusive

    const Box<Dims> selectionBox = helper::StartEndBox(
        variable.m_Start, variable.m_Count, m_ReverseDimensions);

    for (size_t step = stepStart; step < stepEnd; ++step)
    {
        auto itBlockStarts =
            variable.m_AvailableStepBlockIndexOffsets.find(step);
        if (itBlockStarts == variable.m_AvailableStepBlockIndexOffsets.end())
        {
            continue;
        }

        const std::vector<size_t> &blockStarts = itBlockStarts->second;

        // blockPosition gets updated by Read, can't be const
        for (size_t blockPosition : blockStarts)
        {
            const Characteristics<T> blockCharacteristics =
                ReadElementIndexCharacteristics<T>(
                    buffer, blockPosition,
                    static_cast<DataTypes>(GetDataType<T>()));

            // check if they intersect
            helper::SubFileInfo info;
            info.BlockBox = helper::StartEndBox(blockCharacteristics.Start,
                                                blockCharacteristics.Count);
            info.IntersectionBox =
                helper::IntersectionBox(selectionBox, info.BlockBox);

            if (info.IntersectionBox.first.empty() ||
                info.IntersectionBox.second.empty())
            {
                continue;
            }
            // if they intersect get info Seeks (first: start, second:
            // count)
            info.Seeks.first =
                blockCharacteristics.Statistics.PayloadOffset +
                helper::LinearIndex(info.BlockBox, info.IntersectionBox.first,
                                    m_IsRowMajor) *
                    sizeof(T);

            info.Seeks.second =
                blockCharacteristics.Statistics.PayloadOffset +
                (helper::LinearIndex(info.BlockBox, info.IntersectionBox.second,
                                     m_IsRowMajor) +
                 1) *
                    sizeof(T);

            const size_t fileIndex =
                static_cast<size_t>(blockCharacteristics.Statistics.FileIndex);

            infoMap[fileIndex][step].push_back(std::move(info));
        }
    }

    return infoMap;
}

// PRIVATE
template <class T>
std::vector<typename core::Variable<T>::Info> BP3Deserializer::BlocksInfoCommon(
    const core::Variable<T> &variable,
    const std::vector<size_t> &blocksIndexOffsets) const
{
    std::vector<typename core::Variable<T>::Info> blocksInfo;
    blocksInfo.reserve(blocksIndexOffsets.size());

    for (const size_t blockIndexOffset : blocksIndexOffsets)
    {
        size_t position = blockIndexOffset;

        const Characteristics<T> blockCharacteristics =
            ReadElementIndexCharacteristics<T>(
                m_Metadata.m_Buffer, position,
                static_cast<DataTypes>(GetDataType<T>()));

        typename core::Variable<T>::Info blockInfo;
        blockInfo.Start = blockCharacteristics.Start;
        blockInfo.Count = blockCharacteristics.Count;

        if (blockCharacteristics.Statistics.IsValue) // value
        {
            blockInfo.IsValue = true;
            blockInfo.Value = blockCharacteristics.Statistics.Value;
        }
        else // array
        {
            blockInfo.IsValue = false;
            blockInfo.Min = blockCharacteristics.Statistics.Min;
            blockInfo.Max = blockCharacteristics.Statistics.Max;
        }
        blocksInfo.push_back(blockInfo);
    }
    return blocksInfo;
}

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP3DESERIALIZER_TCC_ */
