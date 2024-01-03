//
// Created by cguo51 on 12/30/23.
//
#ifndef KVCACHECOMMON_TCC
#define KVCACHECOMMON_TCC

namespace adios2
{

void KVCacheCommon::openConnection()
{
    m_redisContext = redisConnect(m_host.c_str(), m_port);
    if (m_redisContext == NULL || m_redisContext->err)
    {
        std::cout << "Error to connect to kvcache server: " << m_redisContext->errstr << std::endl;
        if (m_redisContext)
        {
            redisFree(m_redisContext);
        }
    }
    else
    {
        std::cout << "------------------------------------------------------------" << std::endl;
        std::cout << "Connected to kvcache server. KV Cache Version Control: V1.0" << std::endl;
    }
}

void KVCacheCommon::closeConnection()
{
    redisFree(m_redisContext);
    std::cout << "KVCache connection closed" << std::endl;
}

template <typename T>
void KVCacheCommon::set(std::string key, const std::vector<T>& vec)
{
    encodeVector(vec, m_value);
    m_command = "SET " + key + " " + m_value;
    m_redisReply = (redisReply *)redisCommand(m_redisContext, m_command.c_str());
    if (m_redisReply == NULL)
    {
        std::cout << "Error to set key: " << key << std::endl;
    }
    else
    {
        std::cout << "SET Key: " << key << " Value size: " << vec.size() << std::endl;
        freeReplyObject(m_redisReply);
    }
}

template <typename T>
void KVCacheCommon::get(std::string key, std::vector<T>& vec)
{
    m_command = "GET " + key;
    m_redisReply = (redisReply *)redisCommand(m_redisContext, m_command.c_str());
    if (m_redisReply == NULL)
    {
        std::cout << "Error to get key: " << key << std::endl;
    }
    else
    {
        decodeVector(m_redisReply->str, vec);
        freeReplyObject(m_redisReply);
    }
}

void KVCacheCommon::del(std::string key)
{
    m_command = "DEL " + key;
    m_redisReply = (redisReply *)redisCommand(m_redisContext, m_command.c_str());
    if (m_redisReply == NULL)
    {
        std::cout << "Error to delete key: " << key << std::endl;
    }
    else
    {
        freeReplyObject(m_redisReply);
    }
}

bool KVCacheCommon::exists(std::string key)
{
    m_command = "EXISTS " + key;
    m_redisReply = (redisReply *)redisCommand(m_redisContext, m_command.c_str());
    if (m_redisReply == NULL)
    {
        std::cout << "The Key: " << key << " does not exist" << std::endl;
        return false;
    }
    else
    {
        if (!m_redisReply->integer)
        {
            std::cout << "The Key: " << key << " does not exist" << std::endl;
            return false;
        }
        freeReplyObject(m_redisReply);
        return true;
    }
}

std::string KVCacheCommon::keyPrefix(char *VarName, size_t AbsStep, size_t BlockID)
{
    return VarName + std::to_string(AbsStep) + std::to_string(BlockID);
}

std::string KVCacheCommon::keyComposition(const std::string &key_prefix, Dims Start, Dims Count)
{
    std::string box = QueryBox::serializeQueryBox(QueryBox{Start, Count});
    std::string cacheKey = key_prefix + box;
    // replace special characters
    std::replace(cacheKey.begin(), cacheKey.end(), '"', '_');
    std::replace(cacheKey.begin(), cacheKey.end(), ',', '_');
    std::replace(cacheKey.begin(), cacheKey.end(), '(', '_');
    std::replace(cacheKey.begin(), cacheKey.end(), ')', '_');
    std::replace(cacheKey.begin(), cacheKey.end(), '[', '_');
    std::replace(cacheKey.begin(), cacheKey.end(), ']', '_');
    std::replace(cacheKey.begin(), cacheKey.end(), '{', '_');
    std::replace(cacheKey.begin(), cacheKey.end(), '}', '_');
    return cacheKey;
}

void KVCacheCommon::keyPrefixExistence(const std::string &key_prefix, std::set<std::string> &keys)
{
    std::string keyPattern = key_prefix + "*";
    m_command = "KEYS " + keyPattern;
    m_redisReply = (redisReply *)redisCommand(m_redisContext, m_command.c_str());
    if (m_redisReply == NULL)
    {
        std::cout << "Error to get keys with prefix: " << key_prefix << std::endl;
    }
    else
    {
        for (int i = 0; i < m_redisReply->elements; i++)
        {
            keys.insert(m_redisReply->element[i]->str);
        }
        freeReplyObject(m_redisReply);
    }
}

template <typename T>
void KVCacheCommon::encodeVector(const std::vector<T>& vec, std::string& encodedString) {
    size_t vecSize = vec.size() * sizeof(T);
    const unsigned char* vecBytes = reinterpret_cast<const unsigned char*>(vec.data());

    size_t encodedSize = vecSize * 3 / 2;
    std::vector<unsigned char> encodedBytes(encodedSize);

    size_t sizeAfterEncoded = adios2sysBase64_Encode(vecBytes, vecSize, encodedBytes.data(), 0);

    // Resize the vector to the actual size
    encodedBytes.resize(sizeAfterEncoded);

    // Convert the encoded bytes to a string
    encodedString.assign(encodedBytes.begin(), encodedBytes.end());
}

template <typename T>
void KVCacheCommon::decodeVector(const std::string& str, std::vector<T>& vec) {
    size_t decodedSize = str.size() * 2;
    std::vector<unsigned char> decodedBytes(decodedSize);

    size_t sizeAfterDecoded = adios2sysBase64_Decode(reinterpret_cast<const unsigned char*>(str.data()), str.size(), decodedBytes.data(), decodedSize);

    // Resize the vector to the actual size
    decodedBytes.resize(sizeAfterDecoded);

    // Copy the decoded bytes to the vector
    vec.resize(sizeAfterDecoded / sizeof(T));
    memcpy(vec.data(), decodedBytes.data(), sizeAfterDecoded);

}

}; // namespace adios2
#endif // KVCACHECOMMON_TCC