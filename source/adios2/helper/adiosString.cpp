/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosString.cpp
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosString.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::transform
#include <fstream>
#include <ios> //std::ios_base::failure
#include <sstream>
#include <stdexcept> // std::invalid_argument
/// \endcond

namespace adios2
{
namespace helper
{

std::string FileToString(const std::string &fileName, const std::string hint)
{
    std::ifstream fileStream(fileName);

    if (!fileStream)
    {
        throw std::ios_base::failure("ERROR: file " + fileName +
                                     " not found, " + hint + "\n");
    }

    std::ostringstream fileSS;
    fileSS << fileStream.rdbuf();
    fileStream.close();
    return fileSS.str();
}

Params BuildParametersMap(const std::vector<std::string> &parameters,
                          const bool debugMode)
{
    auto lf_GetFieldValue = [](const std::string parameter, std::string &field,
                               std::string &value, const bool debugMode) {
        auto equalPosition = parameter.find("=");

        if (debugMode)
        {
            if (equalPosition == parameter.npos)
            {
                throw std::invalid_argument(
                    "ERROR: wrong format for IO parameter " + parameter +
                    ", format must be key=value for each entry \n");
            }

            if (equalPosition == parameter.size() - 1)
            {
                throw std::invalid_argument(
                    "ERROR: empty value in IO parameter " + parameter +
                    ", format must be key=value \n");
            }
        }

        field = parameter.substr(0, equalPosition);
        value = parameter.substr(equalPosition + 1); // need to test
    };

    // BODY OF FUNCTION STARTS HERE
    Params parametersOutput;

    for (const auto parameter : parameters)
    {
        std::string field, value;
        lf_GetFieldValue(parameter, field, value, debugMode);

        if (debugMode)
        {
            if (parametersOutput.count(field) == 1)
            {
                throw std::invalid_argument(
                    "ERROR: parameter " + field +
                    " already exists, must be unique\n");
            }
        }

        parametersOutput[field] = value;
    }

    return parametersOutput;
}

std::string AddExtension(const std::string &name,
                         const std::string extension) noexcept
{
    std::string result(name);
    if (name.find(extension) != name.size() - 3)
    {
        result += extension;
    }
    return result;
}

bool EndsWith(const std::string &str, const std::string &ending,
              const bool caseSensitive)
{
    if (str.length() >= ending.length())
    {
        if (caseSensitive)
        {
            return (!str.compare(str.length() - ending.length(),
                                 ending.length(), ending));
        }
        else
        {
            std::string strLC = std::string(str);
            std::string endLC = std::string(ending);
            std::transform(strLC.begin(), strLC.end(), strLC.begin(),
                           ::tolower);
            std::transform(endLC.begin(), endLC.end(), endLC.begin(),
                           ::tolower);
            return (!strLC.compare(strLC.length() - endLC.length(),
                                   endLC.length(), endLC));
        }
    }
    else
    {
        return false;
    }
};

std::vector<std::string>
GetParametersValues(const std::string &key,
                    const std::vector<Params> &parametersVector) noexcept
{
    std::vector<std::string> values;
    values.reserve(parametersVector.size());

    for (const auto &parameters : parametersVector)
    {
        auto itKey = parameters.find(key);
        std::string value;
        if (itKey != parameters.end())
        {
            value = itKey->second;
        }
        values.push_back(value);
    }

    return values;
}

void SetParameterValue(const std::string key, const Params &parameters,
                       std::string &value) noexcept
{
    auto itKey = parameters.find(key);
    if (itKey != parameters.end())
    {
        value = itKey->second;
    }
}

std::string GetParameter(const std::string key, const Params &params,
                         const bool isMandatory, const bool debugMode,
                         const std::string hint)
{
    std::string value;
    auto itParameter = params.find(key);
    if (itParameter == params.end())
    {
        if (debugMode && isMandatory)
        {
            throw std::invalid_argument("ERROR: mandatory parameter " + key +
                                        " not found, " + hint);
        }
    }
    else
    {
        value = itParameter->second;
    }
    return value;
}

void SetParameterValueInt(const std::string key, const Params &parameters,
                          int &value, const bool debugMode,
                          const std::string hint)
{
    auto itKey = parameters.find(key);

    if (itKey == parameters.end())
    {
        return;
    }

    if (debugMode)
    {
        try
        {
            value = std::stoi(itKey->second);
        }
        catch (...)
        {
            std::throw_with_nested(std::invalid_argument(
                "ERROR: could not cast " + itKey->second +
                " to int from key parameter: " + itKey->first + ", " + hint));
        }
    }
    else
    {
        value = std::stoi(itKey->second);
    }
}

double StringToDouble(const std::string value, const bool debugMode,
                      const std::string hint)
{
    double valueDouble = -1.;

    if (debugMode)
    {
        try
        {
            valueDouble = std::stod(value);
        }
        catch (...)
        {
            std::throw_with_nested(std::invalid_argument(
                "ERROR: could not cast " + value + " to double, " + hint));
        }
    }
    else
    {
        valueDouble = std::stod(value);
    }
    return valueDouble;
}

unsigned int StringToUInt(const std::string value, const bool debugMode,
                          const std::string hint)
{
    unsigned int valueUInt = 0;

    if (debugMode)
    {
        try
        {
            valueUInt = static_cast<unsigned int>(std::stoul(value));
        }
        catch (...)
        {
            std::throw_with_nested(
                std::invalid_argument("ERROR: could not cast " + value +
                                      " to unsigned int, " + hint));
        }
    }
    else
    {
        valueUInt = static_cast<unsigned int>(std::stoul(value));
    }
    return valueUInt;
}

std::string DimsToString(const Dims &dimensions)
{
    std::string dimensionsString("Dims(" + std::to_string(dimensions.size()) +
                                 "):[");

    for (const auto dimension : dimensions)
    {
        dimensionsString += std::to_string(dimension) + ", ";
    }
    dimensionsString.pop_back();
    dimensionsString.pop_back();
    dimensionsString += "]";
    return dimensionsString;
}

std::string GlobalName(const std::string &localName, const std::string &prefix,
                       const std::string separator) noexcept
{
    if (prefix.empty())
    {
        return localName;
    }

    return prefix + separator + localName;
}

} // end namespace helper
} // end namespace adios2
