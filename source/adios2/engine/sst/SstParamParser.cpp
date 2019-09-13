
#include <memory>

#include "SstParamParser.h"
#include "adios2/helper/adiosFunctions.h"

#include "adios2/toolkit/sst/sst.h"

using namespace adios2::core;

void SstParamParser::ParseParams(IO &io, struct _SstParams &Params)
{
    auto lf_SetBoolParameter = [&](const std::string key, int &parameter) {
        auto itKey = io.m_Parameters.find(key);
        if (itKey != io.m_Parameters.end())
        {
            std::string value = itKey->second;
            std::transform(value.begin(), value.end(), value.begin(),
                           ::tolower);
            if (value == "yes" || value == "true" || value == "on")
            {
                parameter = 1;
            }
            else if (value == "no" || value == "false" || value == "off")
            {
                parameter = 0;
            }
            else
            {
                throw std::invalid_argument(
                    "ERROR: Unknown Sst Boolean parameter \"" + value + "\"");
            }
        }
    };
    auto lf_SetIntParameter = [&](const std::string key, int &parameter) {
        auto itKey = io.m_Parameters.find(key);
        if (itKey != io.m_Parameters.end())
        {
            parameter = std::stoi(itKey->second);
            return true;
        }
        return false;
    };

    auto lf_SetStringParameter = [&](const std::string key, char *&parameter) {
        auto itKey = io.m_Parameters.find(key);
        if (itKey != io.m_Parameters.end())
        {
            parameter = strdup(itKey->second.c_str());
            return true;
        }
        return false;
    };

    auto lf_SetRegMethodParameter = [&](const std::string key,
                                        size_t &parameter) {
        auto itKey = io.m_Parameters.find(key);
        if (itKey != io.m_Parameters.end())
        {
            std::string method = itKey->second;
            std::transform(method.begin(), method.end(), method.begin(),
                           ::tolower);
            if (method == "file")
            {
                parameter = SstRegisterFile;
            }
            else if (method == "screen")
            {
                parameter = SstRegisterScreen;
            }
            else if (method == "cloud")
            {
                parameter = SstRegisterCloud;
                throw std::invalid_argument("ERROR: Sst RegistrationMethod "
                                            "\"cloud\" not yet implemented");
            }
            else
            {
                throw std::invalid_argument(
                    "ERROR: Unknown Sst RegistrationMethod parameter \"" +
                    method + "\"");
            }
            return true;
        }
        return false;
    };

    auto lf_SetCompressionMethodParameter = [&](const std::string key,
                                                size_t &parameter) {
        auto itKey = io.m_Parameters.find(key);
        if (itKey != io.m_Parameters.end())
        {
            std::string method = itKey->second;
            std::transform(method.begin(), method.end(), method.begin(),
                           ::tolower);
            if (method == "zfp")
            {
                parameter = SstCompressZFP;
            }
            else if (method == "none")
            {
                parameter = SstCompressNone;
            }
            else
            {
                throw std::invalid_argument(
                    "ERROR: Unknown Sst CompressionMethod parameter \"" +
                    method + "\"");
            }
            return true;
        }
        return false;
    };

    // not really a parameter, but a convenient way to pass this around
    auto lf_SetIsRowMajorParameter = [&](const std::string key,
                                         int &parameter) {
        parameter = adios2::helper::IsRowMajor(io.m_HostLanguage);
        return true;
    };

    auto lf_SetMarshalMethodParameter = [&](const std::string key,
                                            size_t &parameter) {
        auto itKey = io.m_Parameters.find(key);
        if (itKey != io.m_Parameters.end())
        {
            std::string method = itKey->second;
            std::transform(method.begin(), method.end(), method.begin(),
                           ::tolower);
            if (method == "ffs")
            {
                parameter = SstMarshalFFS;
            }
            else if (method == "bp")
            {
                parameter = SstMarshalBP;
            }
            else
            {
                throw std::invalid_argument(
                    "ERROR: Unknown Sst MarshalMethod parameter \"" + method +
                    "\"");
            }
            return true;
        }
        return false;
    };

    auto lf_SetCPCommPatternParameter = [&](const std::string key,
                                            size_t &parameter) {
        auto itKey = io.m_Parameters.find(key);
        if (itKey != io.m_Parameters.end())
        {
            std::string method = itKey->second;
            std::transform(method.begin(), method.end(), method.begin(),
                           ::tolower);
            if (method == "min")
            {
                parameter = SstCPCommMin;
            }
            else if (method == "peer")
            {
                parameter = SstCPCommPeer;
            }
            else
            {
                throw std::invalid_argument(
                    "ERROR: Unknown Sst CPCommPattern parameter \"" + method +
                    "\"");
            }
            return true;
        }
        return false;
    };

    auto lf_SetQueueFullPolicyParameter = [&](const std::string key,
                                              size_t &parameter) {
        auto itKey = io.m_Parameters.find(key);
        if (itKey != io.m_Parameters.end())
        {
            std::string method = itKey->second;
            std::transform(method.begin(), method.end(), method.begin(),
                           ::tolower);
            if (method == "block")
            {
                parameter = SstQueueFullBlock;
            }
            else if (method == "discard")
            {
                parameter = SstQueueFullDiscard;
            }
            else
            {
                throw std::invalid_argument(
                    "ERROR: Unknown Sst QueueFullPolicy parameter \"" + method +
                    "\"");
            }
            return true;
        }
        return false;
    };

    auto lf_SetSpecPreloadModeParameter = [&](const std::string key,
                                              int &parameter) {
        auto itKey = io.m_Parameters.find(key);
        if (itKey != io.m_Parameters.end())
        {
            std::string method = itKey->second;
            std::transform(method.begin(), method.end(), method.begin(),
                           ::tolower);
            if (method == "off")
            {
                parameter = SpecPreloadOff;
            }
            else if (method == "on")
            {
                parameter = SpecPreloadOn;
            }
            else if (method == "auto")
            {
                parameter = SpecPreloadAuto;
            }
            else
            {
                throw std::invalid_argument(
                    "ERROR: Unknown Sst SpeculativePreloadMode parameter \"" +
                    method + "\"");
            }
            return true;
        }
        return false;
    };

#define get_params(Param, Type, Typedecl, Default)                             \
    Params.Param = Default;                                                    \
    lf_Set##Type##Parameter(#Param, Params.Param);
    SST_FOREACH_PARAMETER_TYPE_4ARGS(get_params);
#undef get_params
}
