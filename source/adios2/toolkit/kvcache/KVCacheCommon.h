//
// Created by cguo51 on 12/30/23.
//

#ifndef ADIOS2_KVCACHECOMMON_H
#define ADIOS2_KVCACHECOMMON_H
#include "QueryBox.h"
#include <cstring> // For memcpy
#include <hiredis/hiredis.h>
#include <string>
#include <vector>

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

    KVCacheCommon(std::string host = "localhost", int port = 6379) : m_host(host), m_port(port){};

    void openConnection();

    void closeConnection();

    void set(const char *key, size_t size, void *data);

    void get(const char *key, size_t size, void *data);

    void del(std::string key);

    bool exists(std::string key);

    std::string keyPrefix(char *VarName, size_t AbsStep, size_t BlockID);

    std::string keyComposition(const std::string &key_prefix, Dims Start, Dims Count);

    void keyPrefixExistence(const std::string &key_prefix, std::set<std::string> &keys);
};

}; // adios2

#endif // ADIOS2_KVCACHECOMMON_H
