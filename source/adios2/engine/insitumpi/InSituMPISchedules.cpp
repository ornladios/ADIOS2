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

int GetNumberOfRequests(const std::map<std::string, helper::SubFileInfoMap>
                            &variablesSubFileInfo) noexcept
{
    int n = 0;
    for (const auto &variableNamePair : variablesSubFileInfo)
    {
        // <writer, <steps, <SubFileInfo>>>
        for (const auto &subFileIndexPair : variableNamePair.second)
        {
            // <steps, <SubFileInfo>>  but there is only one step
            for (const auto &stepPair : subFileIndexPair.second)
            {
                // <SubFileInfo>
                for (const auto &sfi : stepPair.second)
                {
                    n++;
                }
            }
        }
    }
    return n;
}

int FixSeeksToZeroOffset(
    std::map<std::string, helper::SubFileInfoMap> &variablesSubFileInfo,
    bool isRowMajor) noexcept
{
    int n = 0;
    for (auto &variableNamePair : variablesSubFileInfo)
    {
        // <writer, <steps, <SubFileInfo>>>
        for (auto &subFileIndexPair : variableNamePair.second)
        {
            // <steps, <SubFileInfo>>  but there is only one step
            for (auto &stepPair : subFileIndexPair.second)
            {
                // <SubFileInfo>
                for (auto &sfi : stepPair.second)
                {
                    FixSeeksToZeroOffset(sfi, isRowMajor);
                    n++;
                }
            }
        }
    }
    return n;
}

void FixSeeksToZeroOffset(helper::SubFileInfo &record, bool isRowMajor) noexcept
{
    size_t nElements = record.Seeks.second - record.Seeks.first + 1;
    size_t pos = helper::LinearIndex(record.BlockBox,
                                     record.IntersectionBox.first, isRowMajor);
    record.Seeks.first = pos;
    record.Seeks.second = pos + nElements - 1;
}

std::map<int, std::vector<char>>
SerializeLocalReadSchedule(const int nWriters,
                           const std::map<std::string, helper::SubFileInfoMap>
                               &variablesSubFileInfo) noexcept
{
    std::map<int, std::vector<char>> buffers;
    std::map<int, int> nVarPerWriter;

    for (const auto &variableNamePair : variablesSubFileInfo)
    {
        const std::string variableName(variableNamePair.first);
        // <writer, <steps, <SubFileInfo>>>
        for (const auto &subFileIndexPair : variableNamePair.second)
        {
            const size_t subFileIndex = subFileIndexPair.first; // writer
            // <steps, <SubFileInfo>>  but there is only one step
            for (const auto &stepPair : subFileIndexPair.second)
            {
                if (buffers.find(subFileIndex) == buffers.end())
                {
                    nVarPerWriter[subFileIndex] = 0;
                    // allocate first 4 bytes (number of requested variables)
                    helper::InsertToBuffer(buffers[subFileIndex],
                                           &nVarPerWriter[subFileIndex], 1);
                }

                // LocalReadSchedule sfi = subFileIndexPair.second[0];
                const std::vector<helper::SubFileInfo> &sfi = stepPair.second;
                SerializeLocalReadSchedule(buffers[subFileIndex], variableName,
                                           sfi);
                ++nVarPerWriter[subFileIndex];
                break; // there is only one step here
            }
        }
    }

    // Record the number of actually requested variables for each buffer
    for (auto &bufferPair : buffers)
    {
        size_t pos = 0;
        const auto peerID = bufferPair.first;
        auto &buffer = bufferPair.second;

        helper::CopyToBuffer(buffer, pos, &nVarPerWriter[peerID]);
    }
    return buffers;
}

void SerializeLocalReadSchedule(std::vector<char> &buffer,
                                const std::string varName,
                                const LocalReadSchedule lrs) noexcept
{
    const int nameLen = static_cast<int>(varName.size());
    helper::InsertToBuffer(buffer, &nameLen, 1);
    helper::InsertToBuffer(buffer, varName.data(), nameLen);
    const int nSubFileInfos = static_cast<int>(lrs.size());
    helper::InsertToBuffer(buffer, &nSubFileInfos, 1);
    for (const auto &blockInfo : lrs)
    {
        SerializeSubFileInfo(buffer, blockInfo);
    }
}

void SerializeSubFileInfo(std::vector<char> &buffer,
                          const helper::SubFileInfo record) noexcept
{
    SerializeBox(buffer, record.BlockBox);
    SerializeBox(buffer, record.IntersectionBox);
    SerializeBox(buffer, record.Seeks);
}

void SerializeBox(std::vector<char> &buffer, const Box<Dims> box) noexcept
{
    const int nDims = static_cast<int>(box.first.size());
    helper::InsertToBuffer(buffer, &nDims, 1);
    helper::InsertToBuffer(buffer, box.first.data(), nDims);
    helper::InsertToBuffer(buffer, box.second.data(), nDims);
}

void SerializeBox(std::vector<char> &buffer, const Box<size_t> box) noexcept
{
    helper::InsertToBuffer(buffer, &box.first, 1);
    helper::InsertToBuffer(buffer, &box.second, 1);
}

int GetNumberOfRequestsInWriteScheduleMap(WriteScheduleMap &map) noexcept
{
    int n = 0;
    //<variable <reader, <SubFileInfo>>>
    for (auto &variableNamePair : map)
    {
        // <reader, <SubFileInfo>>
        for (auto &readerPair : variableNamePair.second)
        {
            // <SubFileInfo>
            for (auto &sfi : readerPair.second)
            {
                n++;
            }
        }
    }
    return n;
}

WriteScheduleMap DeserializeReadSchedule(
    const std::map<int, std::vector<char>> &buffers) noexcept
{
    WriteScheduleMap map;

    for (const auto &bufferPair : buffers)
    {
        const auto peerID = bufferPair.first;
        const auto &buffer = bufferPair.second;

        LocalReadScheduleMap lrsm = DeserializeReadSchedule(buffer);
        for (const auto &varSchedule : lrsm)
        {
            map[varSchedule.first][peerID] = varSchedule.second;
        }
    }
    return map;
}

LocalReadScheduleMap
DeserializeReadSchedule(const std::vector<char> &buffer) noexcept
{
    LocalReadScheduleMap map;
    size_t pos = 0;
    int nVars = helper::ReadValue<int>(buffer, pos);
    for (int i = 0; i < nVars; i++)
    {
        int nameLen = helper::ReadValue<int>(buffer, pos);
        std::vector<char> name(nameLen + 1, '\0');
        helper::CopyFromBuffer(buffer, pos, name.data(), nameLen);
        int nSubFileInfos = helper::ReadValue<int>(buffer, pos);
        std::vector<helper::SubFileInfo> sfis;
        sfis.reserve(nSubFileInfos);
        for (int j = 0; j < nSubFileInfos; j++)
        {
            sfis.push_back(DeserializeSubFileInfo(buffer, pos));
        }
        map[name.data()] = sfis;
    }
    return map;
}

helper::SubFileInfo DeserializeSubFileInfo(const std::vector<char> &buffer,
                                           size_t &position) noexcept
{
    helper::SubFileInfo record;
    record.BlockBox = DeserializeBoxDims(buffer, position);
    record.IntersectionBox = DeserializeBoxDims(buffer, position);
    record.Seeks = DeserializeBoxSizet(buffer, position);
    return record;
}

Box<Dims> DeserializeBoxDims(const std::vector<char> &buffer,
                             size_t &position) noexcept
{
    Box<Dims> box;
    int nDims = helper::ReadValue<int>(buffer, position);
    std::vector<size_t> start(nDims);
    std::vector<size_t> count(nDims);
    helper::CopyFromBuffer(buffer, position, start.data(), nDims);
    helper::CopyFromBuffer(buffer, position, count.data(), nDims);
    return Box<Dims>(start, count);
}

Box<size_t> DeserializeBoxSizet(const std::vector<char> &buffer,
                                size_t &position) noexcept
{
    size_t first, second;
    helper::CopyFromBuffer(buffer, position, &first, 1);
    helper::CopyFromBuffer(buffer, position, &second, 1);
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
            std::cout << "{ reader = " << readerPair.first << " ";
            const std::vector<helper::SubFileInfo> &sfis = readerPair.second;
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

void PrintSubFileInfo(const helper::SubFileInfo &sfi) noexcept
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
    for (size_t i = 0; i < dims.size(); i++)
    {
        std::cout << dims[i];
        if (i < dims.size() - 1)
        {
            std::cout << ",";
        }
    }
    std::cout << "}";
}

} // end namespace insitumpi

} // end namespace adios2
