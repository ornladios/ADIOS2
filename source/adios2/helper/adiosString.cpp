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
#include "adiosString.tcc"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::transform
#include <fstream>
#include <ios> //std::ios_base::failure
#include <sstream>
#include <stdexcept> // std::invalid_argument
/// \endcond

#include "adios2/helper/adiosType.h" //BytesFactor

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
                          const char delimKeyValue, const bool debugMode)
{
    auto lf_Trim = [](std::string &input) {
        input.erase(0, input.find_first_not_of(" \n\r\t")); // prefixing spaces
        input.erase(input.find_last_not_of(" \n\r\t") + 1); // suffixing spaces
    };

    auto lf_GetFieldValue = [](const std::string parameter, std::string &field,
                               std::string &value, const char delimKeyValue,
                               const bool debugMode) {
        auto equalPosition = parameter.find(delimKeyValue);

        if (debugMode)
        {
            if (equalPosition == parameter.npos)
            {
                throw std::invalid_argument(
                    "ERROR: wrong format for IO parameter " + parameter +
                    ", format must be key" + delimKeyValue +
                    "value for each entry \n");
            }
        }

        field = parameter.substr(0, equalPosition);
        value = parameter.substr(equalPosition + 1);
    };

    // BODY OF FUNCTION STARTS HERE
    Params parametersOutput;

    for (const std::string &parameter : parameters)
    {
        std::string field, value;
        lf_GetFieldValue(parameter, field, value, delimKeyValue, debugMode);
        lf_Trim(field);
        lf_Trim(value);

        if (debugMode)
        {
            if (value.length() == 0)
            {
                throw std::invalid_argument(
                    "ERROR: empty value in IO parameter " + parameter +
                    ", format must be key" + delimKeyValue + "value \n");
            }
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

Params BuildParametersMap(const std::string &input, const char delimKeyValue,
                          const char delimItem, const bool debugMode)
{
    auto lf_Trim = [](std::string &input) {
        input.erase(0, input.find_first_not_of(" \n\r\t")); // prefixing spaces
        input.erase(input.find_last_not_of(" \n\r\t") + 1); // suffixing spaces
    };

    Params parametersOutput;

    std::istringstream inputSS(input);
    std::string parameter;
    while (std::getline(inputSS, parameter, delimItem))
    {
        const size_t position = parameter.find(delimKeyValue);
        if (debugMode && position == parameter.npos)
        {
            throw std::invalid_argument(
                "ERROR: wrong format for IO parameter " + parameter +
                ", format must be key" + delimKeyValue +
                "value for each entry \n");
        }

        std::string key = parameter.substr(0, position);
        lf_Trim(key);
        std::string value = parameter.substr(position + 1);
        lf_Trim(value);
        if (debugMode)
        {
            if (value.length() == 0)
            {
                throw std::invalid_argument(
                    "ERROR: empty value in IO parameter " + parameter +
                    ", format must be key" + delimKeyValue + "value \n");
            }
            if (parametersOutput.count(key) == 1)
            {
                throw std::invalid_argument(
                    "ERROR: key " + key +
                    " appears multiple times in the parameters string\n");
            }
        }

        parametersOutput[key] = value;
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
            const std::string strLC = LowerCase(str);
            const std::string endLC = LowerCase(ending);

            return (!strLC.compare(strLC.length() - endLC.length(),
                                   endLC.length(), endLC));
        }
    }
    else
    {
        return false;
    }
}

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
                          const std::string &hint)
{
    auto itKey = parameters.find(key);
    if (itKey == parameters.end())
    {
        // try lower case
        const std::string keyLC = LowerCase(key);

        itKey = parameters.find(keyLC);
        if (itKey == parameters.end())
        {
            return;
        }
    }

    value = static_cast<int>(StringTo<int32_t>(itKey->second, debugMode, hint));
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

Dims StringToDims(const std::string &dimensions)
{
    std::vector<size_t> shape;
    size_t begin = 0;
    for (size_t end = 0; end < dimensions.size(); ++end)
    {
        if (dimensions[end] == ',')
        {
            std::string s(dimensions, begin, end - begin);
            shape.push_back(stoull(s));
            begin = end + 1;
            end = begin;
        }
    }
    std::string s(dimensions, begin, dimensions.size() - begin);
    shape.push_back(stoull(s));
    return shape;
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

size_t StringToSizeT(const std::string &input, const bool debugMode,
                     const std::string &hint)
{
    if (sizeof(size_t) == sizeof(uint32_t))
    {
        return StringTo<uint32_t>(input, debugMode, hint);
    }

    return StringTo<uint64_t>(input, debugMode, hint);
}

size_t StringToByteUnits(const std::string &input, const bool debugMode,
                         const std::string &hint)
{
    std::string units;
    size_t unitsLength = 2;

    if (EndsWith(input, "gb", true))
    {
        units = "gb";
    }
    else if (EndsWith(input, "mb", true))
    {
        units = "mb";
    }
    else if (EndsWith(input, "kb", true))
    {
        units = "kb";
    }
    else if (EndsWith(input, "b", true))
    {
        units = "b";
        unitsLength = 1;
    }

    const std::string number(input.substr(0, input.size() - unitsLength));
    const size_t factor = BytesFactor(units, debugMode);

    return static_cast<size_t>(std::stoul(number) * factor);
}

std::string LowerCase(const std::string &input)
{
    std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(), ::tolower);
    return output;
}

std::set<std::string>
PrefixMatches(const std::string &prefix,
              const std::set<std::string> &inputs) noexcept
{
    std::set<std::string> outputs;
    auto itPrefix = inputs.lower_bound(prefix);

    while (itPrefix != inputs.end())
    {
        const std::string &input = *itPrefix;
        // check if it's an actual prefix
        if (input.compare(0, prefix.size(), prefix) == 0)
        {
            outputs.insert(input);
        }
        else
        {
            break;
        }
        ++itPrefix;
    }
    return outputs;
}

} // end namespace helper
} // end namespace adios2
