/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosType.cpp
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosType.h"
#include "adiosType.inl"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::transform, std::count
#include <sstream>
/// \endcond

namespace adios
{

std::string DimsToCSV(const Dims &dimensions) noexcept
{
    std::string dimsCSV;

    for (const auto dimension : dimensions)
    {
        dimsCSV += std::to_string(dimension) + ",";
    }

    if (!dimsCSV.empty())
    {
        dimsCSV.pop_back(); // remove last comma
    }

    return dimsCSV;
}

std::vector<int> CSVToVectorInt(const std::string csv) noexcept
{
    std::vector<int> numbers;
    if (csv.empty())
    {
        return numbers;
    }

    if (csv.find(",") == csv.npos) // if no commas, one int
    {
        numbers.push_back(std::stoi(csv)); // might need to be checked
    }
    else
    {
        int count = std::count(csv.begin(), csv.end(), ',');
        numbers.reserve(count);

        std::istringstream csvSS(csv);
        std::string value;
        while (std::getline(csvSS, value, ','))
        {
            numbers.push_back(std::stoi(csv));
        }
    }

    return numbers;
}

void ConvertUint64VectorToSizetVector(const std::vector<uint64_t> &in,
                                      std::vector<size_t> &out) noexcept
{
    out.resize(in.size());
    std::transform(in.begin(), in.end(), out.begin(),
                   [](uint64_t value) { return static_cast<size_t>(value); });
}

void Uint64ArrayToSizetVector(const size_t nElements, const uint64_t *in,
                              std::vector<size_t> &out) noexcept
{
    out.resize(nElements);
    for (size_t i = 0; i < nElements; i++)
    {
        out[i] = static_cast<size_t>(in[i]);
    }
}

std::vector<std::size_t> Uint64ArrayToSizetVector(const size_t nElements,
                                                  const uint64_t *in) noexcept
{
    std::vector<size_t> out(nElements);
    for (size_t i = 0; i < nElements; i++)
    {
        out[i] = static_cast<size_t>(in[i]);
    }
    return out;
}

std::vector<std::size_t>
Uint64VectorToSizetVector(const std::vector<uint64_t> &in) noexcept
{
    std::vector<size_t> out(in.size());
    out.resize(in.size());
    std::transform(in.begin(), in.end(), out.begin(),
                   [](uint64_t value) { return static_cast<size_t>(value); });

    return out;
}

TimeUnit StringToTimeUnit(const std::string timeUnitString,
                          const bool debugMode)
{
    TimeUnit timeUnit = TimeUnit::Microseconds; // default

    if (timeUnitString == "Microseconds")
    {
        timeUnit = TimeUnit::Microseconds;
    }
    else if (timeUnitString == "Milliseconds")
    {
        timeUnit = TimeUnit::Milliseconds;
    }
    else if (timeUnitString == "Seconds")
    {
        timeUnit = TimeUnit::Seconds;
    }
    else if (timeUnitString == "Minutes")
    {
        timeUnit = TimeUnit::Minutes;
    }
    else if (timeUnitString == "Hours")
    {
        timeUnit = TimeUnit::Hours;
    }
    else
    {
        if (debugMode)
        {
            throw std::invalid_argument("ERROR: profile_units=value "
                                        " must be Microseconds, Milliseconds, "
                                        "Seconds, Minutes or Hours\n");
        }
    }
    return timeUnit;
}

size_t BytesFactor(const std::string units, const bool debugMode)
{
    size_t factor = 1; // bytes
    if (units == "Gb")
    {
        factor = 1024 * 1024 * 1024;
    }
    else if (units == "Mb")
    {
        factor = 1024 * 1024;
    }
    else if (units == "Kb")
    {
        factor = 1024;
    }
    else if (units == "b" || units == "bytes")
    {
        // do nothing
    }
    else
    {
        if (debugMode)
        {
            throw std::invalid_argument("ERROR: units " + units +
                                        " not supported\n");
        }
    }
    return factor;
}

std::string OpenModeToString(const OpenMode openMode,
                             const bool oneLetter) noexcept
{
    std::string openModeString;

    if (openMode == OpenMode::Write)
    {
        if (oneLetter)
        {
            openModeString = "w";
        }
        else
        {
            openModeString = "Write";
        }
    }
    else if (openMode == OpenMode::Append)
    {
        if (oneLetter)
        {
            openModeString = "a";
        }
        else
        {
            openModeString = "Append";
        }
    }
    else if (openMode == OpenMode::Read)
    {
        if (oneLetter)
        {
            openModeString = "r";
        }
        else
        {
            openModeString = "Read";
        }
    }
    return openModeString;
}

} // end namespace adios
