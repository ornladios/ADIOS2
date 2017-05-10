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

    if (dimsCSV.empty() == false)
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
    TimeUnit timeUnit = TimeUnit::MicroSeconds; // default

    if (timeUnitString == "mus" || timeUnitString == "microseconds")
    {
        timeUnit = TimeUnit::mus;
    }
    else if (timeUnitString == "ms" || timeUnitString == "milliseconds")
    {
        timeUnit = TimeUnit::ms;
    }
    else if (timeUnitString == "s" || timeUnitString == "seconds")
    {
        timeUnit = TimeUnit::s;
    }
    else if (timeUnitString == "m" || timeUnitString == "minutes")
    {
        timeUnit = TimeUnit::m;
    }
    else if (timeUnitString == "h" || timeUnitString == "hours")
    {
        timeUnit = TimeUnit::h;
    }
    else
    {
        if (debugMode == true)
        {
            throw std::invalid_argument("ERROR: profile_units=value "
                                        " must be mus, ms, s, m or h\n");
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
        if (debugMode == true)
        {
            throw std::invalid_argument("ERROR: units " + units +
                                        " not supported\n");
        }
    }
    return factor;
}

std::string OpenModeToString(const OpenMode openMode) noexcept
{

    std::string openModeString;
    if (openMode == OpenMode::Write || openMode == OpenMode::w)
    {
        openModeString = "w";
    }
    else if (openMode == OpenMode::Append || openMode == OpenMode::a)
    {
        openModeString = "a";
    }
    else if (openMode == OpenMode::Read || openMode == OpenMode::r)
    {
        openModeString = "r";
    }
    return openModeString;
}

} // end namespace adios
