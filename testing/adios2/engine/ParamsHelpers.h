/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_ADIOS2_ENGINE_PARAMS_HELPERS_H_
#define TEST_ADIOS2_ENGINE_PARAMS_HELPERS_H_

#include <adios2.h>

#include <cctype>
#include <stdexcept>
#include <string>

static std::string Trim(std::string &str)
{
    if (str != "")
    {
        size_t first = str.find_first_not_of(' ');
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }
    return str;
}

/*
 * Engine parameters spec is a poor-man's JSON.  name=value pairs are separated
 * by commas.  White space is trimmed off front and back.
 *
 * Values containing special characters can be quoted with single or double quotes:
 *   DataTransport=awssdk,S3Endpoint="http://127.0.0.1:9000",S3Bucket=mybucket
 */
static adios2::Params ParseEngineParams(std::string Input)
{
    adios2::Params Ret = {};
    size_t pos = 0;
    size_t len = Input.length();

    while (pos < len)
    {
        // Skip leading whitespace
        while (pos < len && std::isspace(Input[pos]))
        {
            ++pos;
        }
        if (pos >= len)
        {
            break;
        }

        // Parse parameter name (up to '=')
        size_t nameStart = pos;
        while (pos < len && Input[pos] != '=')
        {
            ++pos;
        }
        if (pos >= len)
        {
            throw std::invalid_argument("Engine parameter missing '=' delimiter");
        }
        std::string paramName = Input.substr(nameStart, pos - nameStart);
        paramName = Trim(paramName);
        ++pos; // skip '='

        // Parse parameter value
        std::string paramValue;

        // Skip whitespace before value
        while (pos < len && std::isspace(Input[pos]))
        {
            ++pos;
        }

        if (pos < len && (Input[pos] == '"' || Input[pos] == '\''))
        {
            // Quoted value - read until matching quote
            char quote = Input[pos];
            ++pos; // skip opening quote
            size_t valueStart = pos;
            while (pos < len && Input[pos] != quote)
            {
                ++pos;
            }
            if (pos >= len)
            {
                throw std::invalid_argument("Engine parameter \"" + paramName +
                                            "\" has unterminated quote");
            }
            paramValue = Input.substr(valueStart, pos - valueStart);
            ++pos; // skip closing quote
        }
        else
        {
            // Unquoted value - read until comma or end
            size_t valueStart = pos;
            while (pos < len && Input[pos] != ',')
            {
                ++pos;
            }
            paramValue = Input.substr(valueStart, pos - valueStart);
            paramValue = Trim(paramValue);
        }

        if (paramName.empty())
        {
            throw std::invalid_argument("Engine parameter has empty name");
        }

        if (paramValue.empty())
        {
            throw std::invalid_argument("Engine parameter has empty value");
        }

        Ret[paramName] = paramValue;

        // Skip comma separator if present
        if (pos < len && Input[pos] == ',')
        {
            ++pos;
        }
    }
    return Ret;
}

#endif // TEST_ADIOS2_ENGINE_PARAMS_HELPERS_H_