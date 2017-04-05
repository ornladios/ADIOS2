/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1.cpp
 *
 *  Created on: Feb 7, 2017
 *      Author: wfg
 */

#include "format/BP1.h"
#include "functions/adiosFunctions.h"

namespace adios
{
namespace format
{

std::string BP1::GetDirectoryName(const std::string name) const noexcept
{
    std::string directory;

    if (name.find(".bp") == name.size() - 3)
    {
        directory = name;
    }
    else
    {
        directory = name + ".bp";
    }
    return directory;
}

void BP1::OpenRankFiles(const std::string name, const std::string accessMode,
                        Transport &file) const
{
    const std::string directory = GetDirectoryName(name);
    CreateDirectory(
        directory); // creates a directory and sub-directories recursively

    std::string fileName(directory + "/" + directory + "." +
                         std::to_string(file.m_RankMPI));
    file.Open(fileName, accessMode); // opens a file transport under
                                     // name.bp.dir/name.bp.rank reserve that
                                     // location fro writing
}

std::vector<std::uint8_t> BP1::GetMethodIDs(
    const std::vector<std::shared_ptr<Transport>> &transports) const noexcept
{
    auto lf_GetMethodID = [](const std::string method) -> std::uint8_t {
        int id = METHOD_UNKNOWN;
        if (method == "NULL")
        {
            id = METHOD_NULL;
        }
        else if (method == "POSIX")
        {
            id = METHOD_POSIX;
        }
        else if (method == "FStream")
        {
            id = METHOD_FSTREAM;
        }
        else if (method == "File")
        {
            id = METHOD_FILE;
        }
        else if (method == "MPI")
        {
            id = METHOD_MPI;
        }

        return id;
    };

    std::vector<std::uint8_t> methodIDs;
    methodIDs.reserve(transports.size());

    for (const auto &transport : transports)
    {
        methodIDs.push_back(lf_GetMethodID(transport->m_Type));
    }

    return methodIDs;
}

} // end namespace format
} // end namespace adios
