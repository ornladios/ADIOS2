//
// Created by cguo51 on 12/30/23.
//

#ifndef ADIOS2_KVCACHECOMMON_H
#define ADIOS2_KVCACHECOMMON_H
#include "QueryBox.h"
#include <cstring> // For memcpy
#include <string>
#include <vector>

#ifdef ADIOS2_HAVE_KVCACHE
#include <hiredis/hiredis.h>
#endif

namespace adios2
{

namespace kvcache
{

class KVCacheCommon
{
#ifdef ADIOS2_HAVE_KVCACHE
private:
    redisContext *m_redisContext;
    redisReply *m_redisReply;

public:
    ~KVCacheCommon() { CloseConnection(); }

    void OpenConnection(std::string host = "localhost", int port = 6379);

    void CloseConnection();

    void Set(const char *key, size_t size, void *data);

    void Get(const char *key, size_t size, void *data);

    // Batch operations in pipeline, mode 0 for SET, 1 for GET
    void AppendCommandInBatch(const char *key, size_t mode, size_t size, void *data);

    void ExecuteBatch(const char *key, size_t mode, size_t size, void *data);

    bool Exists(std::string key);

    void KeyPrefixExistence(const std::string &key_prefix, std::unordered_set<std::string> &keys);
#else
public:
    KVCacheCommon() = default;
    ~KVCacheCommon() = default;
    void OpenConnection(){};
    void CloseConnection(){};
    void AppendCommandInBatch(const char *key, size_t mode, size_t size, void *data){};
    void ExecuteBatch(const char *key, size_t mode, size_t size, void *data){};
    bool Exists(std::string key) { return false; };
    void KeyPrefixExistence(const std::string &key_prefix, std::unordered_set<std::string> &keys){};

#endif /* ADIOS2_HAVE_KVCACHE */
};
}; // namespace kvcache
}; // adios2

#endif // ADIOS2_KVCACHECOMMON_H
