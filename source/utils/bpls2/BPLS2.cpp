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
#include "adios2/ADIOSMacros.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/IO.h"
#include "adios2/engine/bp/BPFileReader.h"
#include "adios2/helper/adiosFunctions.h"

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
        throw std::invalid_argument(
            "ERROR: Missing adios2 bp file or arguments\n" + m_HelpMessage);
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
        }
        throw std::invalid_argument("");
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
    auto lf_PrintVerboseHeader = [](const BPFileReader &bpFileReader) {

        const std::map<std::string, Params> variablesInfo =
            bpFileReader.m_IO.GetAvailableVariables();

        const std::map<std::string, Params> attributesInfo =
            bpFileReader.m_IO.GetAvailableAttributes();

        const auto &metadataSet = bpFileReader.m_BP3Deserializer.m_MetadataSet;
        std::cout << "File info:\n";
        std::cout << "  groups:     " << metadataSet.DataPGCount << "\n";
        std::cout << "  variables:  " << variablesInfo.size() << "\n";
        std::cout << "  attributes: " << attributesInfo.size() << "\n";
        std::cout << "  meshes:     TODO\n";
        std::cout << "  steps:      " << metadataSet.StepsCount << "\n";
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

    auto lf_PrintVariables = [&](BPFileReader &bpFileReader) {

        const std::map<std::string, Params> &variablesInfo =
            bpFileReader.m_IO.GetAvailableVariables();

        size_t maxTypeSize = 0;
        size_t maxNameSize = 0;
        for (const auto &variablePair : variablesInfo)
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

        for (const auto &variablePair : variablesInfo)
        {
            const std::string name(variablePair.first);
            const Params &variableParameters = variablePair.second;
            const std::string type(variableParameters.at("Type"));

            std::cout << "  ";
            std::cout << std::left << std::setw(maxTypeSize) << type << "  ";
            std::cout << std::left << std::setw(maxNameSize) << name << "  ";

            // print min max
            if (m_Parameters.count("long") == 1)
            {
                if (variableParameters.at("SingleValue") == "false")
                {
                    std::cout << variableParameters.at("AvailableStepsCount")
                              << "*{" << variableParameters.at("Shape")
                              << "}  ";
                    std::cout << variableParameters.at("Min") << " / "
                              << variableParameters.at("Max");
                }
                else
                {
                    std::cout << variableParameters.at("AvailableStepsCount")
                              << "*scalar " << variableParameters.at("Value");
                }
            }
            std::cout << "\n";
        }
        std::cout << std::endl;
    };

    auto lf_PrintAttributes = [&](IO &io) {
        const std::map<std::string, Params> &attributesInfo =
            io.GetAvailableAttributes();

        size_t maxTypeSize = 0;
        size_t maxNameSize = 0;
        for (const auto &attributePair : attributesInfo)
        {
            const size_t nameSize = attributePair.first.size();
            if (nameSize > maxNameSize)
            {
                maxNameSize = nameSize;
            }

            const Params &parameters = attributePair.second;
            const size_t typeSize = parameters.at("Type").size();
            if (typeSize > maxTypeSize)
            {
                maxTypeSize = typeSize;
            }
        }

        for (const auto &attributePair : attributesInfo)
        {
            const std::string name(attributePair.first);
            const Params &parameters = attributePair.second;
            const std::string type(parameters.at("Type"));

            std::cout << "  ";
            std::cout << std::left << std::setw(maxTypeSize) << type << "  ";
            std::cout << std::left << std::setw(maxNameSize) << name << "  ";
            std::cout << "attribute = " << parameters.at("Value");
            std::cout << "\n";
        }
        std::cout << std::endl;
    };

    ADIOS adios(true);
    IO &io = adios.DeclareIO("bpls2");
    BPFileReader bpFileReader(io, m_FileName, Mode::Read, io.m_MPIComm);

    if (m_Parameters.count("verbose") == 1)
    {
        lf_PrintVerboseHeader(bpFileReader);
    }

    if (m_Parameters.count("attrsonly") == 0)
    {
        lf_PrintVariables(bpFileReader);
    }

    if (m_Parameters.count("attrs") == 1 ||
        m_Parameters.count("attrsonly") == 1)
    {
        lf_PrintAttributes(io);
    }
}

} // end namespace utils
} // end namespace adios2
