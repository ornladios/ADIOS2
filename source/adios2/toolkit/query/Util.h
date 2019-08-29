#ifndef ADIOS2_QUERY_UTIL_H
#define ADIOS2_QUERY_UTIL_H

#include <cctype>
#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <stdexcept>
#include <string>
#include <vector>

namespace adios2
{
namespace query
{
static size_t ToUIntValue(const adios2::Params &params, const std::string &key,
                          size_t defaultValue)
{
    auto it = params.find(key);
    if (it != params.end())
    {
        try
        {
            auto value = (size_t)(std::stoul(it->second));
            return value;
        }
        catch (std::exception &e)
        {
            std::cout << e.what() << std::endl;
            return defaultValue;
        }
    }
    return defaultValue;
}

static bool ToBoolValue(const adios2::Params &params, const std::string &key)
{
    auto keyPair = params.find(key);
    if (keyPair != params.end())
    {
        std::string value = keyPair->second;
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (value == "yes" || value == "true")
        {
            return true;
        }
    }
    return false;
}

static bool EndsWith(const std::string &hostStr, const std::string &fileTag)
{
    if (hostStr.size() >= fileTag.size() &&
        hostStr.compare(hostStr.size() - fileTag.size(), fileTag.size(),
                        fileTag) == 0)
        return true;
    else
        return false;
}

static bool IsFileNameXML(const std::string &filename)
{
    return EndsWith(filename, ".xml");
}

static bool IsFileNameJSON(const std::string &filename)
{
    return EndsWith(filename, ".json");
}
}; // namespace query
}; // name space adios2

#endif // QUERY_WORKER_H
