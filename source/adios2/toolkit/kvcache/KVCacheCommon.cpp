//
// Created by cguo51 on 12/30/23.
//
#ifndef KVCACHECOMMON_CPP
#define KVCACHECOMMON_CPP

#include "KVCacheCommon.h"

#include <cstring>

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
    if (m_CacheFile.is_open())
    {
        m_CacheFile.close();
    }
}

void KVCacheCommon::SetLocalCacheFile(const std::string localCacheFilePath)
{
    m_LocalCacheFilePath = localCacheFilePath;
    // open/create file only when required
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

constexpr size_t MAX_SIZE_INSIDE_KV = 1024 * 1024;

void KVCacheCommon::AppendSetCommandInBatch(const char *key, size_t size, void *data)
{
    // Write data to cache (reply in ExecuteBatch)
    if (size > MAX_SIZE_INSIDE_KV)
    {
        // save data to file and add reference in key-value
        if (!m_CacheFile.is_open())
        {
            m_CacheFile.open(m_LocalCacheFilePath, std::ios::in | std::ios::out | std::ios::app);
        }
        m_CacheFile.seekp(0, std::ios_base::end);
        const std::streampos offset = m_CacheFile.tellp();
        m_CacheFile.write(static_cast<char *>(data), static_cast<std::streamsize>(size));
        std::string value =
            "fileblock:offset=" + std::to_string(offset) + ":size=" + std::to_string(size);
        redisAppendCommand(m_redisContext, "SET %s %b", key, value.c_str(), value.size());
    }
    else
    {
        redisAppendCommand(m_redisContext, "SET %s %b", key, data, size);
    }
}

void KVCacheCommon::ExecuteSetBatch(const char *key)
{
    if (redisGetReply(m_redisContext, (void **)&m_redisReply) == REDIS_OK)
    {
        freeReplyObject(m_redisReply);
    }
    else
    {
        std::cout << "Error to execute batch command: " << key << std::endl;
    }
}

void KVCacheCommon::AppendGetCommandInBatch(const char *key)
{
    // Read data from cache (combo with ExecuteBatch)
    redisAppendCommand(m_redisContext, "GET %s", key);
}

void KVCacheCommon::ExecuteGetBatch(const char *key, size_t size, void *data)
{
    if (redisGetReply(m_redisContext, (void **)&m_redisReply) == REDIS_OK)
    {
        if (!std::strncmp("fileblock:offset=", m_redisReply->str, 17))
        {
            size_t cOffset, cSize;

            char *saveptr;
            char *endptr;
            char *token = strtok_r(m_redisReply->str, ":", &saveptr); // fileblock

            token = strtok_r(NULL, ":", &saveptr); // offset=%d
            cOffset = strtoull(token + 7, &endptr, 10);

            token = strtok_r(NULL, ":", &saveptr); // size=%d
            cSize = strtoull(token + 5, &endptr, 10);

            token = strtok_r(NULL, ":", &saveptr);
            if (token)
            {
                std::cout << "Cache Error: extra characters found in key-value pair "
                             "pointing to cached data on disk. key = "
                          << key << " offset = " << cOffset << " size = " << cSize << ", rest = ["
                          << token << "]" << std::endl;
            }

            if (size != cSize)
            {
                std::cout << "Cache Error: expected block size = " << size
                          << " but cache value says size = " << cSize << " for key = " << key
                          << std::endl;
            }

            // data is in the cache file
            if (!m_CacheFile.is_open())
            {
                m_CacheFile.open(m_LocalCacheFilePath,
                                 std::ios::in | std::ios::out | std::ios::app);
                if (!m_CacheFile)
                {
                    std::cout << "Cache Error: File Open Error details: " << strerror(errno)
                              << std::endl;
                }
            }
            m_CacheFile.seekg(cOffset, std::ios_base::beg);
            if (!m_CacheFile)
            {
                std::cout << "Cache Error: Seek Error details: " << strerror(errno) << std::endl;
            }
            errno = 0;
            m_CacheFile.read(static_cast<char *>(data), cSize);
            if (m_CacheFile.fail())
            {
                std::cout << "Cache Error: when reading " << cSize << " bytes from cache file "
                          << m_LocalCacheFilePath << " from offset " << cOffset
                          << " error: " << strerror(errno) << std::endl;
            }
        }
        else
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
            // std::cout << "The Key: " << key << " does not exist" << std::endl;
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

void KVCacheCommon::RemotePathHashMd5(const std::string &remotePath, std::string &result)
{
    adios2sysMD5 *md5 = adios2sysMD5_New();
    if (!md5)
    {
        throw std::runtime_error("Failed to create MD5 instance");
    }

    // Initialize the MD5 instance
    adios2sysMD5_Initialize(md5);

    // Update the MD5 instance with the input data
    adios2sysMD5_Append(md5, reinterpret_cast<const unsigned char *>(remotePath.c_str()),
                        remotePath.size());

    // Finalize the MD5 digest and get the hash value
    unsigned char digest[16];
    adios2sysMD5_Finalize(md5, digest);

    // Convert the digest to a hexadecimal string
    char hexDigest[32];
    adios2sysMD5_DigestToHex(digest, hexDigest);

    // Clean up the MD5 instance
    adios2sysMD5_Delete(md5);

    // from 0 to 31
    result = std::string(hexDigest, 32);
}
};     // namespace kvcache
};     // namespace adios2
#endif // KVCACHECOMMON_CPP