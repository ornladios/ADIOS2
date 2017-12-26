/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPISchedules.cpp
 * common functions for in situ MPI Writer and Reader
 * It requires MPI
 *
 *  Created on: Dec 25, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "InSituMPISchedules.h"

#include "adios2/helper/adiosMath.h"
#include "adios2/helper/adiosMemory.h"

#include <iostream>

namespace adios2
{

namespace insitumpi
{

void FixSeeksToZeroOffset(
    std::map<std::string, SubFileInfoMap> &variablesSubFileInfo) noexcept
{
    for (auto &variableNamePair : variablesSubFileInfo)
    {
        // <writer, <steps, SubFileInfo>>
        for (auto &subFileIndexPair : variableNamePair.second)
        {
            // <steps, SubFileInfo>  but there is only one step
            for (auto &stepPair : subFileIndexPair.second)
            {
                // <SubFileInfo>
                for (auto &sfi : stepPair.second)
                {
                    FixSeeksToZeroOffset(sfi);
                }
            }
        }
    }
}

void FixSeeksToZeroOffset(SubFileInfo &record) noexcept
{
    size_t nElements = record.Seeks.second - record.Seeks.first + 1;
    size_t pos =
        LinearIndex(record.BlockBox, record.IntersectionBox.first, true);
    record.Seeks.first = pos;
    record.Seeks.second = pos + nElements - 1;
}

std::vector<std::vector<char>> SerializeLocalReadSchedule(
    const int nWriters,
    const std::map<std::string, SubFileInfoMap> &variablesSubFileInfo) noexcept
{
    std::vector<std::vector<char>> buffers(nWriters);

    // Create a buffer for each writer
    std::vector<int> nVarPerWriter(nWriters);
    for (size_t i = 0; i < nWriters; i++)
    {
        nVarPerWriter[i] = 0;
        InsertToBuffer(buffers[i], &nVarPerWriter[i],
                       1); // allocate first 4 bytes
    }

    for (const auto &variableNamePair : variablesSubFileInfo)
    {
        const std::string variableName(variableNamePair.first);
        // <writer, <steps, SubFileInfo>>
        for (const auto &subFileIndexPair : variableNamePair.second)
        {
            const size_t subFileIndex = subFileIndexPair.first; // writer
            auto &lrs = buffers[subFileIndex];
            // <steps, SubFileInfo>  but there is only one step
            for (const auto &stepPair : subFileIndexPair.second)
            {
                // LocalReadSchedule sfi = subFileIndexPair.second[0];
                const std::vector<SubFileInfo> &sfi = stepPair.second;
                SerializeLocalReadSchedule(buffers[subFileIndex], variableName,
                                           sfi);
                ++nVarPerWriter[subFileIndex];
                break; // there is only one step here
            }
        }
    }

    // Record the number of actually requested variables for each buffer
    for (size_t i = 0; i < nWriters; i++)
    {
        size_t pos = 0;
        CopyToBuffer(buffers[i], pos, &nVarPerWriter[i]);
    }
    return buffers;
}

void SerializeLocalReadSchedule(std::vector<char> &buffer,
                                const std::string varName,
                                const LocalReadSchedule lrs) noexcept
{
    const int nameLen = varName.size();
    InsertToBuffer(buffer, &nameLen, 1);
    InsertToBuffer(buffer, varName.data(), nameLen);
    const int nSubFileInfos = lrs.size();
    InsertToBuffer(buffer, &nSubFileInfos, 1);
    for (const auto &blockInfo : lrs)
    {
        SerializeSubFileInfo(buffer, blockInfo);
    }
}

void SerializeSubFileInfo(std::vector<char> &buffer,
                          const SubFileInfo record) noexcept
{
    SerializeBox(buffer, record.BlockBox);
    SerializeBox(buffer, record.IntersectionBox);
    SerializeBox(buffer, record.Seeks);
}

void SerializeBox(std::vector<char> &buffer, const Box<Dims> box) noexcept
{
    const int nDims = box.first.size();
    InsertToBuffer(buffer, &nDims, 1);
    InsertToBuffer(buffer, box.first.data(), nDims);
    InsertToBuffer(buffer, box.second.data(), nDims);
}

void SerializeBox(std::vector<char> &buffer, const Box<size_t> box) noexcept
{
    InsertToBuffer(buffer, &box.first, 1);
    InsertToBuffer(buffer, &box.second, 1);
}

WriteScheduleMap
DeserializeReadSchedule(const std::vector<std::vector<char>> &buffers) noexcept
{
    WriteScheduleMap map;

    for (int i = 0; i < buffers.size(); i++)
    {
        const auto &buffer = buffers[i];
        LocalReadScheduleMap lrsm = DeserializeReadSchedule(buffer);
        for (const auto &varSchedule : lrsm)
        {
            map[varSchedule.first][i] = varSchedule.second;
        }
    }
    return map;
}

LocalReadScheduleMap
DeserializeReadSchedule(const std::vector<char> &buffer) noexcept
{
    LocalReadScheduleMap map;
    size_t pos = 0;
    int nVars = ReadValue<int>(buffer, pos);
    for (int i = 0; i < nVars; i++)
    {
        int nameLen = ReadValue<int>(buffer, pos);
        char name[nameLen + 1];
        CopyFromBuffer(buffer, pos, name, nameLen);
        name[nameLen] = '\0';
        int nSubFileInfos = ReadValue<int>(buffer, pos);
        std::vector<SubFileInfo> sfi;
        sfi.reserve(nSubFileInfos);
        for (int j = 0; j < nSubFileInfos; j++)
        {
            sfi.push_back(DeserializeSubFileInfo(buffer, pos));
        }
        map[name] = sfi;
    }
    return map;
}

SubFileInfo DeserializeSubFileInfo(const std::vector<char> &buffer,
                                   size_t &position) noexcept
{
    SubFileInfo record;
    record.BlockBox = DeserializeBoxDims(buffer, position);
    record.IntersectionBox = DeserializeBoxDims(buffer, position);
    record.Seeks = DeserializeBoxSizet(buffer, position);
    return record;
}

Box<Dims> DeserializeBoxDims(const std::vector<char> &buffer,
                             size_t &position) noexcept
{
    Box<Dims> box;
    int nDims = ReadValue<int>(buffer, position);
    std::vector<size_t> start(nDims);
    std::vector<size_t> count(nDims);
    CopyFromBuffer(buffer, position, start.data(), nDims);
    CopyFromBuffer(buffer, position, count.data(), nDims);
    return Box<Dims>(start, count);
}

Box<size_t> DeserializeBoxSizet(const std::vector<char> &buffer,
                                size_t &position) noexcept
{
    size_t first, second;
    CopyFromBuffer(buffer, position, &first, 1);
    CopyFromBuffer(buffer, position, &second, 1);
    return Box<size_t>(first, second);
}

void PrintReadScheduleMap(const WriteScheduleMap &map) noexcept
{
    // <variableName, <reader, <SubFileInfo>>>
    for (const auto &variableNamePair : map)
    {
        std::cout << "{ var = " << variableNamePair.first << " ";
        // <reader, <SubFileInfo>>
        for (const auto &readerPair : variableNamePair.second)
        {
            const size_t readerRank = readerPair.first;
            std::cout << "{ reader = " << readerPair.first << " ";
            const std::vector<SubFileInfo> &sfis = readerPair.second;
            for (const auto &sfi : sfis)
            {
                std::cout << "<";
                PrintSubFileInfo(sfi);
                std::cout << "> ";
            }
            std::cout << "} ";
        }
        std::cout << "} ";
    }
}

void PrintSubFileInfo(const SubFileInfo &sfi) noexcept
{
    std::cout << "(block=";
    PrintBox(sfi.BlockBox);
    std::cout << ", intersection=";
    PrintBox(sfi.IntersectionBox);
    std::cout << ", seeks=";
    PrintBox(sfi.Seeks);
    std::cout << ")";
}

void PrintBox(const Box<Dims> &box) noexcept
{
    std::cout << "[";
    PrintDims(box.first);
    std::cout << ",";
    PrintDims(box.second);
    std::cout << "]";
}

void PrintBox(const Box<size_t> &box) noexcept
{
    std::cout << "{" << box.first << "," << box.second << "}";
}

void PrintDims(const Dims &dims) noexcept
{
    std::cout << "{";
    for (int i = 0; i < dims.size(); i++)
    {
        std::cout << dims[i];
        if (i < dims.size() - 1)
            std::cout << ",";
    }
    std::cout << "}";
}

} // end namespace insitumpi

} // end namespace adios2
