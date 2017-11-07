/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPLS2.cpp
 *
 *  Created on: Oct 5, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPLS2.h"

#include <iomanip>
#include <iostream>

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/IO.h"
#include "adios2/engine/bp/BPFileReader.h"

namespace adios2
{
namespace utils
{

const std::string BPLS2::m_HelpMessage = "For usage run either:\n"
                                         "\t bpls2 --help\n"
                                         "\t bpls2 -h \n";

const std::map<std::string, std::string> BPLS2::m_Options = {
    {"long", "l"}, {"attrs", "a"},   {"attrsonly", "A"},
    {"dump", "d"}, {"verbose", "v"}, {"help", "h"}};

BPLS2::BPLS2(int argc, char *argv[]) : Utils("bpls2", argc, argv) {}

void BPLS2::Run()
{
    ParseArguments();
    ProcessParameters();
    ProcessTransport();
}

// PRIVATE
void BPLS2::ParseArguments()
{
    if (m_Arguments.size() == 1)
    {
        throw std::invalid_argument("ERROR: Missing bpfile\n" + m_HelpMessage);
    }

    bool isFileSet = false;

    for (auto itArg = m_Arguments.begin() + 1; itArg != m_Arguments.end();
         ++itArg)
    {
        if (itArg->find("--") == 0) // long  argument
        {
            const std::string argument(itArg->substr(2));
            if (argument.size() == 1)
            {
                throw std::invalid_argument(
                    "ERROR: unknown single character option, did you mean -" +
                    argument + " ?\n");
            }
            SetParameters(argument, true);
        }
        else if (itArg->find("-") == 0) // short argument
        {
            const std::string argument(itArg->substr(1));
            SetParameters(argument, false);
            if (argument == "s" || argument == "c")
            {
                ++itArg;
                // SetParameters(*itArg);
            }
        }
        else
        {
            if (isFileSet)
            {
                throw std::invalid_argument(
                    "ERROR: only 1 bpfile is allowed\n" + m_HelpMessage);
            }

            m_FileName = *itArg;
            isFileSet = true;
        }
    }
}

void BPLS2::ProcessParameters() const
{
    if (m_Parameters.count("help") == 1)
    {
        PrintUsage();
        if (m_Parameters.size() > 1)
        {
            std::cout << "\n";
            std::cout << "Found --help , -h option, discarding others\n";
            std::cout << "Rerun without --help , -h option\n";
            throw std::invalid_argument("");
        }
    }

    if (m_FileName.empty())
    {
        throw std::invalid_argument("ERROR: file not passed to bpls2\n" +
                                    m_HelpMessage);
    }
}

void BPLS2::PrintUsage() const noexcept
{
    std::cout << "This is ADIOS2 binary pack listing (bpls2) utility. Usage:\n";
    std::cout << "\t bpls2 [OPTIONS] bpfile [MASK1 MASK2 ...]\n";
    std::cout << "\n";
    std::cout << "[OPTIONS]:\n";
    std::cout << "\n";
    std::cout << "-l , --long        Print variables and attributes metadata\n";
    std::cout << "                   information, no overhead\n";
    std::cout << "-a , --attributes  List attributes metadata\n";
    std::cout << "-v , --verbose     Added file information\n";
    std::cout << "\n";
    std::cout << "Example: bpls2 -lav bpfile" << std::endl;
}

void BPLS2::PrintExamples() const noexcept {}

void BPLS2::SetParameters(const std::string argument, const bool isLong)
{
    if (argument == "-" || argument.empty())
    {
        throw std::invalid_argument("ERROR: invalid argument: -" + argument +
                                    "\n");
    }

    bool isOption = false;

    if (m_Options.count(argument) == 1 && isLong)
    {
        isOption = true;
        m_Parameters[argument] = "";
    }
    else if (!isLong)
    {
        for (const auto &optionPair : m_Options)
        {
            if (argument == optionPair.second)
            {
                isOption = true;
                m_Parameters[optionPair.first] = "";
                break;
            }
        }
    }

    if (isOption)
    {
        return;
    }

    // look for multiple options by character
    for (const char argChar : argument)
    {
        const std::string argCharString(1, argChar);
        isOption = false;

        for (const auto &optionPair : m_Options)
        {
            if (argCharString == optionPair.second)
            {
                m_Parameters[optionPair.first] = "";
                isOption = true;
            }
        }

        if (!isOption)
        {
            throw std::invalid_argument("ERROR: unknown option " +
                                        argCharString + " in argument " +
                                        argument + "\n");
        }
    }
}

void BPLS2::ProcessTransport() const
{
    auto lf_PrintVerboseHeader = [](const BPFileReader &bpFileReader,
                                    const size_t variablesSize,
                                    const size_t attributesSize) {

        const auto &metadataSet = bpFileReader.m_BP3Deserializer.m_MetadataSet;
        std::cout << "File info:\n";
        std::cout << "  groups:     " << metadataSet.DataPGCount << "\n";
        std::cout << "  variables:  " << variablesSize << "\n";
        std::cout << "  attributes: " << attributesSize << "\n";
        std::cout << "  meshes:     TODO\n";
        std::cout << "  steps:      " << metadataSet.TimeStep << "\n";
        std::cout << "  file size:  "
                  << bpFileReader.m_FileManager.GetFileSize(0) << " bytes\n";

        const auto &minifooter = bpFileReader.m_BP3Deserializer.m_Minifooter;
        std::cout << "  bp version: " << std::to_string(minifooter.Version)
                  << "\n";
        std::string endianness("Little Endian");
        if (!minifooter.IsLittleEndian)
        {
            endianness = "Big Endian";
        }
        std::cout << "  endianness: " << endianness << "\n";
        std::cout << "  statistics: Min / Max\n";
        std::cout << "\n";
    };

    auto lf_PrintVariables =
        [&](const std::map<std::string, Params> &variablesMap)

    {
        // get maximum sizes
        size_t maxTypeSize = 0;
        size_t maxNameSize = 0;
        for (const auto &variablePair : variablesMap)
        {
            const size_t nameSize = variablePair.first.size();
            if (nameSize > maxNameSize)
            {
                maxNameSize = nameSize;
            }

            const Params &parameters = variablePair.second;
            const size_t typeSize = parameters.at("Type").size();
            if (typeSize > maxTypeSize)
            {
                maxTypeSize = typeSize;
            }
        }

        for (const auto &variablePair : variablesMap)
        {
            const std::string name(variablePair.first);
            const Params &parameters = variablePair.second;
            const std::string type(parameters.at("Type"));

            std::cout << "  ";
            std::cout << std::left << std::setw(maxTypeSize) << type << "  ";
            std::cout << std::left << std::setw(maxNameSize) << name << "  ";

            // print min max
            if (m_Parameters.count("long") == 1)
            {
                std::cout << parameters.at("Min") << " / "
                          << parameters.at("Max");
            }
            std::cout << "\n";
        }
        std::cout << std::endl;
    };

    ADIOS adios(true);
    IO &io = adios.DeclareIO("bpls2");
    BPFileReader bpFileReader(io, m_FileName, Mode::Read, io.m_MPIComm);
    const std::map<std::string, Params> variablesMap =
        io.GetAvailableVariables();
    // const auto attributesMap = io.GetAvailableAttributes();

    if (m_Parameters.count("verbose") == 1)
    {
        lf_PrintVerboseHeader(bpFileReader, variablesMap.size(), 0);
    }

    lf_PrintVariables(variablesMap);
}

} // end namespace utils
} // end namespace adios2
