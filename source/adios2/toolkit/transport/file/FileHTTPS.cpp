#include "FileHTTPS.h"
#include "adios2/helper/adiosString.h"
#include <adios2sys/SystemTools.hxx>

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace adios2
{
namespace transport
{

void ParseURL(const std::string &url, std::string &hostname, std::string &path)
{
    const std::string httpsPrefix = "https://";
    size_t pos = 0;

    // Skip scheme if present
    if (url.rfind(httpsPrefix, 0) == 0)
        pos = httpsPrefix.size();

    // Find first '/' after hostname
    size_t slashPos = url.find('/', pos);
    if (slashPos == std::string::npos)
    {
        hostname = url.substr(pos);
        path = "";
    }
    else
    {
        hostname = url.substr(pos, slashPos - pos);
        path = url.substr(slashPos);
    }
}

std::string ExtractHeaderValue(const std::string &headers, const std::string &key)
{
    size_t pos = headers.find(key);
    if (pos == std::string::npos)
        return "";
    pos += key.size();
    size_t end = headers.find("\r\n", pos);
    return headers.substr(pos, end - pos);
}

FileHTTPS::FileHTTPS(helper::Comm const &comm) : Transport("File", "HTTPS", comm) {}

FileHTTPS::~FileHTTPS() { Close(); }

void FileHTTPS::SetParameters(const Params &params)
{
    // Parameters are set from config parameters if present
    // Otherwise, they remain at their default value

    helper::SetParameterValue("cache", params, m_CachePath);
    helper::SetParameterValue("hostname", params, m_hostname);
    helper::SetParameterValue("path", params, m_path);
    helper::SetParameterValueInt("verbose", params, m_Verbose, "");

    std::string recheckStr = "true";
    helper::SetParameterValue("recheck_metadata", params, recheckStr);
    m_RecheckMetadata = helper::StringTo<bool>(recheckStr, "");

    if (m_Verbose > 0)
    {
        std::cout << "FileHTTPS::SetParameters: HTTPS Transport created host =" << m_hostname
                  << " path = " << m_path << " local cache = '" << m_CachePath << "'"
                  << " recheck_metadata = " << m_RecheckMetadata << std::endl;
    }
}

void FileHTTPS::WaitForOpen()
{
    if (m_IsOpening)
    {
        m_IsOpening = false;
        CheckFile("couldn't open HTTPS socket " + m_Name);
        m_IsOpen = true;
    }
}

void FileHTTPS::SetUpCache()
{
    if (!m_CachePath.empty())
    {
        if (helper::EndsWith(m_path, "md.idx"))
        {
            m_CachingThisFile = true;
            m_CacheFilePath = m_CachePath + "/md.idx";
        }
        else if (helper::EndsWith(m_path, "mmd.0"))
        {
            m_CachingThisFile = true;
            m_CacheFilePath = m_CachePath + "/mmd.0";
        }
        else if (helper::EndsWith(m_path, "md.0"))
        {
            m_CachingThisFile = true;
            m_CacheFilePath = m_CachePath + "/md.0";
        }
    }

    if (m_CachingThisFile)
    {
        if (!m_RecheckMetadata)
        {
            m_CacheFileRead = new FileFStream(m_Comm);
            try
            {
                m_CacheFileRead->Open(m_CacheFilePath, Mode::Read);
                m_Size = m_CacheFileRead->GetSize();
                m_IsCached = true;
                m_CachingThisFile = false;
                if (m_Verbose > 0)
                {
                    std::cout << "FileHTTPS::SetUpCache: Already cached " << m_CacheFilePath
                              << ", size = " << m_Size << std::endl;
                }
            }
            catch (std::ios_base::failure &)
            {
                delete m_CacheFileRead;
                m_IsCached = false;
            }
        }
    }
}

void FileHTTPS::CheckCache(const size_t fileSize)
{
    if (m_CachingThisFile)
    {
        /* Check if cache file exists and size equals the cloud version*/
        m_CacheFileRead = new FileFStream(m_Comm);
        try
        {
            m_CacheFileRead->Open(m_CacheFilePath, Mode::Read);
            size_t cacheSize = m_CacheFileRead->GetSize();
            if (cacheSize >= fileSize)
            {
                m_IsCached = true;
                m_CachingThisFile = false;
                if (m_Verbose > 0)
                {
                    std::cout << "FileHTTPS::CheckCache: Already cached " << m_CacheFilePath
                              << ", full size = " << cacheSize << std::endl;
                }
            }
            else
            {
                std::cout << "FileHTTPS::CheckCache: Partially cached " << m_CacheFilePath
                          << ", cached size = " << cacheSize << " full size = " << fileSize
                          << std::endl;
            }
        }
        catch (std::ios_base::failure &)
        {
            delete m_CacheFileRead;
        }

        if (m_CachingThisFile)
        {
            /* Create output file for caching this data later */
            const auto lastPathSeparator(m_CacheFilePath.find_last_of(PathSeparator));
            if (lastPathSeparator != std::string::npos)
            {
                const std::string dirpath(m_CacheFilePath.substr(0, lastPathSeparator));
                adios2sys::SystemTools::MakeDirectory(dirpath);
                // Cannot call this on Windows because it confuses it with CreateDirectoryA()
                // helper::CreateDirectory(dirpath);
            }
            m_CacheFileWrite = new FileFStream(m_Comm);
            m_CacheFileWrite->Open(m_CacheFilePath, Mode::Write);
            if (m_Verbose > 0)
            {
                std::cout << "FileHTTPS::CheckCache: Caching turn on for " << m_CacheFilePath
                          << std::endl;
            }
        }
    }
}

void FileHTTPS::Open(const std::string &name, const Mode openMode, const bool async,
                     const bool directio)
{
    if (m_hostname.empty())
    {
        ParseURL(name, m_hostname, m_path);
    }
    if (m_Verbose)
    {
        std::cout << "FileHTTPS::Open( hostname = " << m_hostname << ", path = " << m_path << ")\n";
    }
    // GetSize();
    m_OpenMode = openMode;
    switch (m_OpenMode)
    {
    case Mode::Read:
    case Mode::ReadRandomAccess: {
        ProfilerStart("open");
        errno = 0;
        SetUpCache();
        // m_IsCached=true if already found in cache and m_RecheckMetadata=false
        // m_CachingThisFile=true if we want caching and m_IsCached=false
        // m_CacheFilePath is the path to the local file in cache
        break;
    }
    case Mode::Write:
    case Mode::Append:
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "Open",
                                              "does not support writing " + m_Name);
        break;
    default:
        CheckFile("unknown open mode {" + std::to_string((int)openMode) + "} for file " + m_Name +
                  ", in call to FileHTTPS open");
    }
}

void FileHTTPS::OpenChain(const std::string &name, Mode openMode, const helper::Comm &chainComm,
                          const bool async, const bool directio)
{
    return;
}

void FileHTTPS::Write(const char *buffer, size_t size, size_t start) { return; }

void FileHTTPS::Read(char *buffer, size_t size, size_t start)
{
    if (m_IsCached)
    {
        m_SeekPos = start;
        m_CacheFileRead->Read(buffer, size, m_SeekPos);
        if (m_Verbose > 0)
        {
            std::cout << "FileHTTPS::Read: Read from cache " << m_CacheFileRead->m_Name
                      << " start = " << m_SeekPos << " size = " << size << std::endl;
        }
        return;
    }

    const size_t BUF_SIZE = 8192;
    std::string request = "GET " + m_path + " HTTP/1.1\r\nHost: " + m_hostname +
                          "\r\nRange: bytes=" + std::to_string(start) + "-" +
                          std::to_string(start + size - 1) + "\r\n\r\n";

    m_ssl.Connect(m_hostname, m_server_port);

    if (m_Verbose > 1)
    {
        std::cout << "Request: [" << request << "]" << std::endl;
    }

    m_ssl.Write(request.c_str(), (int)request.size());

    // first we have to read and parse the header to get to the actual data bytes
    bool headerParsed = false;
    std::string response;
    int bytes;
    char buf[BUF_SIZE];
    size_t bytes_recd = 0;
    while (!headerParsed)
    {
        bytes = m_ssl.Read(buf, sizeof(buf));
        response.append(buf, bytes);
        size_t headerEnd = response.find("\r\n\r\n");
        if (headerEnd != std::string::npos)
        {
            headerParsed = true;
            size_t bodyStart = headerEnd + 4;
            size_t bodyLength = response.size() - bodyStart;
            memcpy(buffer, response.data() + bodyStart, bodyLength);
            bytes_recd = bodyLength;
        }
    }

    while (bytes_recd < size)
    {
        int read_len = (int)((size - bytes_recd) < BUF_SIZE ? (size - bytes_recd) : BUF_SIZE);
        int nbytes = m_ssl.Read(buffer + bytes_recd, read_len);
        if (nbytes <= 0)
        {
            helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "Read",
                                                  "SSL_read failed");
        }
        bytes_recd += nbytes;
    }
    if (m_Verbose > 0)
    {
        std::cout << "FileHTTPS::Read Downloaded " << bytes_recd << " bytes.\n";
    }
    /* Save to cache */
    if (m_CachingThisFile)
    {
        m_CacheFileWrite->Write(buffer, size, m_SeekPos);
        m_CacheFileWrite->Flush();
        if (m_Verbose > 0)
        {
            std::cout << "FileHTTPS::Read: Written to cache " << m_CacheFileWrite->m_Name
                      << " start = " << m_SeekPos << " size = " << size << std::endl;
        }
    }
    m_ssl.Close();
}

size_t FileHTTPS::GetSize()
{
    m_ssl.Connect(m_hostname, m_server_port);

    std::string request =
        "HEAD " + m_path + " HTTP/1.1\r\nHost: " + m_hostname + "\r\nConnection: close\r\n\r\n";

    if (m_Verbose > 1)
    {
        std::cout << "Request: [" << request << "]" << std::endl;
    }
    m_ssl.Write(request.c_str(), (int)request.size());

    char buffer[4096] = {0};
    int nbytes = m_ssl.Read(buffer, sizeof(buffer) - 1);
    buffer[nbytes] = '\0';
    if (nbytes <= 0)
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "GetSize",
                                              "failed to read HEAD response");
    }

    std::string headers(buffer);
    if (m_Verbose > 1)
    {
        std::cout << "Headers: " << headers << "\n";
    }
    std::string cl = ExtractHeaderValue(headers, "Content-Length: ");
    m_fileSize = cl.empty() ? 0 : std::stoul(cl);
    if (m_Verbose > 0)
    {
        std::cout << "File size: " << m_fileSize << " bytes\n";
    }

    m_ssl.Close();
    return m_fileSize;
}

// void FileHTTPS::Flush() {}
// void FileHTTPS::Close() {}
// void FileHTTPS::Delete() {}
void FileHTTPS::SeekToEnd() { m_SeekPos = MaxSizeT; }
void FileHTTPS::SeekToBegin() { m_SeekPos = 0; }
void FileHTTPS::Seek(const size_t start)
{
    if (start != MaxSizeT)
    {
        m_SeekPos = start;
    }
    else
    {
        SeekToEnd();
    }
}
void FileHTTPS::Truncate(const size_t length)
{
    helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "Truncate",
                                          "does not support truncating " + m_Name);
}
void FileHTTPS::MkDir(const std::string &fileName) {}

void FileHTTPS::CheckFile(const std::string hint) const
{
    if (m_fileSize == 0)
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "CheckFile",
                                              hint + SysErrMsg());
    }
}

std::string FileHTTPS::SysErrMsg() const
{
    return std::string(": errno = " + std::to_string(m_Errno) + ": " + strerror(m_Errno));
}

} // end namespace transport
} // end namespace adios2
