//
// Created by cguo51 on 12/30/23.
//
#ifndef KVCACHECOMMON_CPP
#define KVCACHECOMMON_CPP

#include "KVCacheCommon.h"

namespace adios2
{
namespace kvcache
{
void KVCacheCommon::OpenConnection(std::string host, int port)
{
    m_redisContext = redisConnect(host.c_str(), port);
    if (m_redisContext == nullptr || m_redisContext->err)
    {
        std::cout << "Error to connect to kvcache server: " << m_redisContext->errstr << std::endl;
        if (m_redisContext)
        {
            redisFree(m_redisContext);
            m_redisContext = nullptr;
        }
    }
    else
    {
        std::cout << "------------------------------------------------------------" << std::endl;
        std::cout << "Connected to kvcache server. KV Cache Version Control: V1.0" << std::endl;
    }
}

void KVCacheCommon::CloseConnection()
{
    if (m_redisContext != nullptr)
    {
        m_redisContext = nullptr;
        std::cout << "KVCache connection closed" << std::endl;
    }
}

void KVCacheCommon::Set(const char *key, size_t size, void *data)
{
    m_redisReply = (redisReply *)redisCommand(m_redisContext, "SET %s %b", key, data, size);
    if (m_redisReply == NULL)
    {
        std::cout << "Error to set key: " << key << std::endl;
    }
    else
    {
        std::cout << "SET Key: " << key << " Value size: " << size << std::endl;
        freeReplyObject(m_redisReply);
    }
}

void KVCacheCommon::Get(const char *key, size_t size, void *data)
{
    m_redisReply = (redisReply *)redisCommand(m_redisContext, "GET %s", key);
    if (m_redisReply == NULL)
    {
        std::cout << "Error to get key: " << key << std::endl;
    }
    else
    {
        memcpy(data, m_redisReply->str, size);
        freeReplyObject(m_redisReply);
    }
}

void KVCacheCommon::AppendCommandInBatch(const char *key, size_t mode, size_t size, void *data)
{
    if (mode == 0)
    {
        redisAppendCommand(m_redisContext, "SET %s %b", key, data, size);
    }
    else if (mode == 1)
    {
        redisAppendCommand(m_redisContext, "GET %s", key);
    }
}

void KVCacheCommon::ExecuteBatch(const char *key, size_t mode, size_t size, void *data)
{
    if (redisGetReply(m_redisContext, (void **)&m_redisReply) == REDIS_OK)
    {
        if (mode == 1)
        {
            memcpy(data, m_redisReply->str, size);
        }
        freeReplyObject(m_redisReply);
    }
    else
    {
        std::cout << "Error to execute batch command: " << key << std::endl;
    }
}

bool KVCacheCommon::Exists(std::string key)
{
    m_redisReply = (redisReply *)redisCommand(m_redisContext, "EXISTS %s", key.c_str());
    if (m_redisReply != NULL)
    {
        if (!m_redisReply->integer)
        {
            std::cout << "The Key: " << key << " does not exist" << std::endl;
            return false;
        }
        freeReplyObject(m_redisReply);
        return true;
    }
    return false;
}

void KVCacheCommon::KeyPrefixExistence(const std::string &key_prefix,
                                       std::unordered_set<std::string> &keys)
{
    m_redisReply = (redisReply *)redisCommand(m_redisContext, "KEYS %s*", key_prefix.c_str());
    if (m_redisReply == NULL)
    {
        std::cout << "Error to get keys with prefix: " << key_prefix << std::endl;
    }
    else
    {
        for (size_t i = 0; i < m_redisReply->elements; i++)
        {
            keys.insert(m_redisReply->element[i]->str);
        }
        freeReplyObject(m_redisReply);
    }
}
};     // namespace kvcache
};     // namespace adios2
#endif // KVCACHECOMMON_CPP