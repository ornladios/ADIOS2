//
// Created by cguo51 on 12/30/23.
//

#ifndef ADIOS2_KVCACHECOMMON_H
#define ADIOS2_KVCACHECOMMON_H
#include <hiredis/hiredis.h>
#include "adios2/toolkit/cache/QueryBox.h"
#include <adios2sys/Base64.h>
#include <cstring>  // For memcpy
#include <vector>
#include <string>

// namespace adios2::KVCache

namespace adios2
{

class KVCacheCommon
{
public:
    std::string m_host;
    int m_port;
    redisContext *m_redisContext;
    redisReply *m_redisReply;
    std::string m_key;
    std::string m_value;
    std::string m_command;

    KVCacheCommon(std::string host="localhost", int port=6379): m_host(host), m_port(port){};

    inline void openConnection();

    inline void closeConnection();

    template <typename T>
    void set(std::string key, const std::vector<T>& vec);

    template <typename T>
    void get(std::string key, std::vector<T>& vec);

    inline void del(std::string key);

    inline bool exists(std::string key);

    inline std::string keyPrefix(char *VarName, size_t AbsStep, size_t BlockID);

    inline std::string keyComposition(const std::string &key_prefix, Dims Start, Dims Count);

    inline void keyPrefixExistence(const std::string &key_prefix, std::set<std::string> &keys);

    template <typename T>
    void encodeVector(const std::vector<T>& vec, std::string& encodedString);

    template <typename T>
    void decodeVector(const std::string& str, std::vector<T>& vec);
};


}; // adios2

#include "KVCacheCommon.tcc"

#endif // ADIOS2_KVCACHECOMMON_H
