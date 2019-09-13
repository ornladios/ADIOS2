/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosString.tcc
 *
 *  Created on: Jun 10, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSSTRING_TCC_
#define ADIOS2_HELPER_ADIOSSTRING_TCC_

#include "adiosString.h"

namespace adios2
{
namespace helper
{

template <>
bool StringTo(const std::string &input, const bool debugMode,
              const std::string &hint)
{
    const std::string value = LowerCase(input);
    bool result = false;

    if (value == "off" || value == "false")
    {
        result = false;
    }
    else if (value == "on" || value == "true")
    {
        result = true;
    }
    else
    {
        if (debugMode)
        {
            throw std::invalid_argument(
                "ERROR: invalid input value: " + input +
                " for on/off or true/false bool conversion " + hint + "\n");
        }
    }
    return result;
}

template <>
int32_t StringTo(const std::string &input, const bool debugMode,
                 const std::string &hint)
{
    if (debugMode)
    {
        try
        {
            const int32_t out = std::stoi(input);
            return out;
        }
        catch (...)
        {
            std::throw_with_nested(std::invalid_argument(
                "ERROR: could not cast " + input + " to int32_t " + hint));
        }
    }
    return std::stoi(input);
}

template <>
uint32_t StringTo(const std::string &input, const bool debugMode,
                  const std::string &hint)
{
    if (debugMode)
    {
        try
        {
            const uint32_t out = static_cast<uint32_t>(std::stoul(input));
            return out;
        }
        catch (...)
        {
            std::throw_with_nested(std::invalid_argument(
                "ERROR: could not cast " + input + " to uint32_t " + hint));
        }
    }
    return std::stoul(input);
}

template <>
int64_t StringTo(const std::string &input, const bool debugMode,
                 const std::string &hint)
{
    if (debugMode)
    {
        try
        {
            const int64_t out = std::stoll(input);
            return out;
        }
        catch (...)
        {
            std::throw_with_nested(std::invalid_argument(
                "ERROR: could not cast " + input + " to int64_t " + hint));
        }
    }
    return std::stoll(input);
}

template <>
uint64_t StringTo(const std::string &input, const bool debugMode,
                  const std::string &hint)
{
    if (debugMode)
    {
        try
        {
            const uint64_t out = std::stoull(input);
            return out;
        }
        catch (...)
        {
            std::throw_with_nested(std::invalid_argument(
                "ERROR: could not cast " + input + " to uint64_t " + hint));
        }
    }
    return std::stoull(input);
}

template <>
float StringTo(const std::string &input, const bool debugMode,
               const std::string &hint)
{
    if (debugMode)
    {
        try
        {
            const float out = std::stof(input);
            return out;
        }
        catch (...)
        {
            std::throw_with_nested(std::invalid_argument(
                "ERROR: could not cast " + input + " to float " + hint));
        }
    }
    return std::stof(input);
}

template <>
double StringTo(const std::string &input, const bool debugMode,
                const std::string &hint)
{
    if (debugMode)
    {
        try
        {
            const double out = std::stod(input);
            return out;
        }
        catch (...)
        {
            std::throw_with_nested(std::invalid_argument(
                "ERROR: could not cast " + input + " to double " + hint));
        }
    }
    return std::stod(input);
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSSTRING_TCC_ */
