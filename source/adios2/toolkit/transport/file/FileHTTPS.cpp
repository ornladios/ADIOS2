/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "FileHTTPS.h"
#include "adios2/helper/adiosString.h"
#include <adios2sys/SystemTools.hxx>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace adios2
{
namespace transport
{

void ParseURL(const std::string &url, std::string &hostname, uint16_t &port,
              std::string &hostHeader, std::string &path)
{
    const std::string httpsPrefix = "https://";
    if (url.rfind(httpsPrefix, 0) != 0)
    {
        helper::Throw<std::invalid_argument>("Toolkit", "transport::file::FileHTTPS", "ParseURL",
                                             "expected an https:// URL, got " + url);
    }

    const size_t authorityBegin = httpsPrefix.size();
    const size_t pathBegin = url.find('/', authorityBegin);
    const std::string authority =
        url.substr(authorityBegin,
                   pathBegin == std::string::npos ? std::string::npos : pathBegin - authorityBegin);
    if (authority.empty())
    {
        helper::Throw<std::invalid_argument>("Toolkit", "transport::file::FileHTTPS", "ParseURL",
                                             "URL has no hostname: " + url);
    }

    std::string portString;
    if (authority.front() == '[')
    {
        const size_t closeBracket = authority.find(']');
        if (closeBracket == std::string::npos)
        {
            helper::Throw<std::invalid_argument>(
                "Toolkit", "transport::file::FileHTTPS", "ParseURL",
                "URL has an invalid bracketed IPv6 address: " + url);
        }
        hostname = authority.substr(1, closeBracket - 1);
        if (closeBracket + 1 < authority.size())
        {
            if (authority[closeBracket + 1] != ':')
            {
                helper::Throw<std::invalid_argument>(
                    "Toolkit", "transport::file::FileHTTPS", "ParseURL",
                    "URL has invalid text after its IPv6 address: " + url);
            }
            portString = authority.substr(closeBracket + 2);
        }
    }
    else
    {
        const size_t colon = authority.rfind(':');
        if (colon != std::string::npos)
        {
            if (authority.find(':') != colon)
            {
                helper::Throw<std::invalid_argument>(
                    "Toolkit", "transport::file::FileHTTPS", "ParseURL",
                    "IPv6 addresses in HTTPS URLs must be enclosed in brackets: " + url);
            }
            hostname = authority.substr(0, colon);
            portString = authority.substr(colon + 1);
        }
        else
        {
            hostname = authority;
        }
    }

    if (hostname.empty())
    {
        helper::Throw<std::invalid_argument>("Toolkit", "transport::file::FileHTTPS", "ParseURL",
                                             "URL has no hostname: " + url);
    }

    port = 443;
    if (!portString.empty())
    {
        size_t parsedCharacters = 0;
        unsigned long parsedPort = 0;
        try
        {
            parsedPort = std::stoul(portString, &parsedCharacters);
        }
        catch (const std::exception &)
        {
            helper::Throw<std::invalid_argument>("Toolkit", "transport::file::FileHTTPS",
                                                 "ParseURL", "URL has an invalid port: " + url);
        }
        if (parsedCharacters != portString.size() || parsedPort == 0 ||
            parsedPort > std::numeric_limits<uint16_t>::max())
        {
            helper::Throw<std::invalid_argument>("Toolkit", "transport::file::FileHTTPS",
                                                 "ParseURL", "URL has an invalid port: " + url);
        }
        port = static_cast<uint16_t>(parsedPort);
    }

    hostHeader = authority;
    path = pathBegin == std::string::npos ? "/" : url.substr(pathBegin);
}

std::string ExtractHeaderValue(const std::string &headers, const std::string &key)
{
    const std::string lowerHeaders = helper::LowerCase(headers);
    const std::string lowerKey = helper::LowerCase(key);
    size_t pos = lowerHeaders.find(lowerKey);
    if (pos == std::string::npos)
        return "";
    pos += key.size();
    const size_t lineEnd = headers.find("\r\n", pos);
    const size_t end = lineEnd == std::string::npos ? headers.size() : lineEnd;
    const size_t first = headers.find_first_not_of(" \t", pos);
    if (first == std::string::npos || first >= end)
        return "";
    const size_t last = headers.find_last_not_of(" \t", end - 1);
    return headers.substr(first, last - first + 1);
}

int HTTPStatusCode(const std::string &headers)
{
    const size_t firstSpace = headers.find(' ');
    if (firstSpace == std::string::npos)
        return 0;
    const size_t secondSpace = headers.find(' ', firstSpace + 1);
    const std::string code = headers.substr(firstSpace + 1, secondSpace == std::string::npos
                                                                ? std::string::npos
                                                                : secondSpace - firstSpace - 1);
    try
    {
        return std::stoi(code);
    }
    catch (const std::exception &)
    {
        return 0;
    }
}

size_t HTTPContentLength(const std::string &headers, const std::string &operation)
{
    const std::string value = ExtractHeaderValue(headers, "Content-Length:");
    try
    {
        size_t parsedCharacters = 0;
        const unsigned long long parsed = std::stoull(value, &parsedCharacters);
        if (parsedCharacters != value.size() || parsed > std::numeric_limits<size_t>::max())
            throw std::out_of_range("content length");
        return static_cast<size_t>(parsed);
    }
    catch (const std::exception &)
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileHTTPS", operation,
            "HTTPS response has an invalid Content-Length header: '" + value + "'");
    }
    return 0;
}

std::string ReadHTTPHeaders(helper::SSLSocket &ssl, std::string &initialBody,
                            const std::string &operation)
{
    constexpr size_t BufferSize = 8192;
    constexpr size_t MaxHeaderSize = 1024 * 1024;
    std::string response;
    char buffer[BufferSize];
    while (true)
    {
        const int bytes = ssl.Read(buffer, sizeof(buffer));
        if (bytes <= 0)
        {
            helper::Throw<std::ios_base::failure>(
                "Toolkit", "transport::file::FileHTTPS", operation,
                "connection closed before the HTTPS response headers were complete");
        }
        response.append(buffer, static_cast<size_t>(bytes));
        const size_t headerEnd = response.find("\r\n\r\n");
        if (headerEnd != std::string::npos)
        {
            initialBody.assign(response, headerEnd + 4, std::string::npos);
            return response.substr(0, headerEnd + 4);
        }
        if (response.size() > MaxHeaderSize)
        {
            helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS",
                                                  operation, "HTTPS response headers exceed 1 MiB");
        }
    }
}

bool MatchesContentRange(const std::string &headers, const size_t expectedStart,
                         const size_t expectedEnd)
{
    const std::string value = ExtractHeaderValue(headers, "Content-Range:");
    std::istringstream stream(value);
    std::string unit;
    std::string range;
    stream >> unit >> range;
    if (helper::LowerCase(unit) != "bytes")
        return false;

    const size_t dash = range.find('-');
    const size_t slash = range.find('/', dash == std::string::npos ? 0 : dash + 1);
    if (dash == std::string::npos || slash == std::string::npos)
        return false;
    try
    {
        size_t startCharacters = 0;
        size_t endCharacters = 0;
        const unsigned long long start = std::stoull(range.substr(0, dash), &startCharacters);
        const std::string endString = range.substr(dash + 1, slash - dash - 1);
        const unsigned long long end = std::stoull(endString, &endCharacters);
        return startCharacters == dash && endCharacters == endString.size() &&
               start == expectedStart && end == expectedEnd;
    }
    catch (const std::exception &)
    {
        return false;
    }
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
    helper::SetParameterValue("ca_file", params, m_CAFile);
    helper::SetParameterValueInt("verbose", params, m_Verbose, "");
    helper::SetParameterValue("filenameintar", params, m_FileNameInTar);

    std::string recheckStr = "true";
    helper::SetParameterValue("recheck_metadata", params, recheckStr);
    m_RecheckMetadata = helper::StringTo<bool>(recheckStr, "");
    m_ssl.SetCAFile(m_CAFile);

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
        std::string path = m_path;
        if (!m_FileNameInTar.empty())
            path = m_FileNameInTar;

        if (helper::EndsWith(path, "md.idx"))
        {
            m_CachingThisFile = true;
            m_CacheFilePath = m_CachePath + "/md.idx";
        }
        else if (helper::EndsWith(path, "mmd.0"))
        {
            m_CachingThisFile = true;
            m_CacheFilePath = m_CachePath + "/mmd.0";
        }
        else if (helper::EndsWith(path, "md.0"))
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
    m_Name = name;
    if (m_hostname.empty())
    {
        ParseURL(name, m_hostname, m_server_port, m_HostHeader, m_path);
    }
    else if (m_HostHeader.empty())
    {
        m_HostHeader = m_hostname;
    }
    if (m_Verbose)
    {
        std::cout << "FileHTTPS::Open( hostname = " << m_hostname << ", port = " << m_server_port
                  << ", path = " << m_path << ")\n";
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
    if (start != MaxSizeT)
    {
        m_SeekPos = start;
    }
    const size_t logicalStart = m_SeekPos;
    const size_t logicalSize = m_BaseSize > 0 ? m_BaseSize : m_fileSize;
    if (logicalSize > 0 && (logicalStart > logicalSize || size > logicalSize - logicalStart))
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "Read",
                                              "logical byte range is outside the HTTPS object " +
                                                  m_Name);
    }
    if (size == 0)
    {
        return;
    }
    if (m_BaseOffset > std::numeric_limits<size_t>::max() - logicalStart)
    {
        helper::Throw<std::overflow_error>("Toolkit", "transport::file::FileHTTPS", "Read",
                                           "HTTPS byte-range start overflows size_t");
    }
    const size_t physicalStart = m_BaseOffset + logicalStart;
    if (size - 1 > std::numeric_limits<size_t>::max() - physicalStart)
    {
        helper::Throw<std::overflow_error>("Toolkit", "transport::file::FileHTTPS", "Read",
                                           "HTTPS byte-range end overflows size_t");
    }
    const size_t physicalEnd = physicalStart + size - 1;

    if (m_IsCached)
    {
        m_CacheFileRead->Read(buffer, size, logicalStart);
        if (m_Verbose > 0)
        {
            std::cout << "FileHTTPS::Read: Read from cache " << m_CacheFileRead->m_Name
                      << " start = " << logicalStart << " size = " << size << std::endl;
        }
        m_SeekPos = logicalStart + size;
        return;
    }

    std::string request = "GET " + m_path + " HTTP/1.1\r\nHost: " + m_HostHeader +
                          "\r\nRange: bytes=" + std::to_string(physicalStart) + "-" +
                          std::to_string(physicalEnd) + "\r\nConnection: close\r\n\r\n";

    try
    {
        m_ssl.Connect(m_hostname, m_server_port);

        if (m_Verbose > 1)
        {
            std::cout << "FileHTTPS::Read Request: [" << request << "]" << std::endl;
        }

        m_ssl.Write(request.c_str(), (int)request.size());

        std::string initialBody;
        const std::string headers = ReadHTTPHeaders(m_ssl, initialBody, "Read");
        const int status = HTTPStatusCode(headers);
        const size_t contentLength = HTTPContentLength(headers, "Read");
        if (contentLength != size)
        {
            helper::Throw<std::ios_base::failure>(
                "Toolkit", "transport::file::FileHTTPS", "Read",
                "HTTPS response length " + std::to_string(contentLength) +
                    " does not match requested length " + std::to_string(size));
        }
        if (status == 206)
        {
            if (!MatchesContentRange(headers, physicalStart, physicalEnd))
            {
                helper::Throw<std::ios_base::failure>(
                    "Toolkit", "transport::file::FileHTTPS", "Read",
                    "HTTPS Content-Range does not match the requested byte range");
            }
        }
        else if (status != 200 || physicalStart != 0)
        {
            helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileHTTPS", "Read",
                                                  "HTTPS range request returned status " +
                                                      std::to_string(status));
        }
        if (initialBody.size() > size)
        {
            helper::Throw<std::ios_base::failure>(
                "Toolkit", "transport::file::FileHTTPS", "Read",
                "HTTPS response body is larger than the requested buffer");
        }

        std::memcpy(buffer, initialBody.data(), initialBody.size());
        size_t bytesReceived = initialBody.size();
        constexpr size_t BufferSize = 8192;
        while (bytesReceived < size)
        {
            const size_t remaining = size - bytesReceived;
            const int readLength = static_cast<int>(std::min(remaining, BufferSize));
            const int bytes = m_ssl.Read(buffer + bytesReceived, readLength);
            if (bytes <= 0)
            {
                helper::Throw<std::ios_base::failure>(
                    "Toolkit", "transport::file::FileHTTPS", "Read",
                    "connection closed before the requested HTTPS range was complete");
            }
            bytesReceived += static_cast<size_t>(bytes);
        }
        if (m_Verbose > 0)
        {
            std::cout << "FileHTTPS::Read Downloaded " << bytesReceived << " bytes.\n";
        }
        if (m_CachingThisFile)
        {
            m_CacheFileWrite->Write(buffer, size, logicalStart);
            m_CacheFileWrite->Flush();
            if (m_Verbose > 0)
            {
                std::cout << "FileHTTPS::Read: Written to cache " << m_CacheFileWrite->m_Name
                          << " start = " << logicalStart << " size = " << size << std::endl;
            }
        }
        m_SeekPos = logicalStart + size;
        m_ssl.Close();
    }
    catch (...)
    {
        m_ssl.Close();
        throw;
    }
}

size_t FileHTTPS::GetSize()
{

    if (m_IsCached && !m_RecheckMetadata)
    {
        return m_BaseSize > 0 ? m_BaseSize : m_Size;
    }

    try
    {
        m_ssl.Connect(m_hostname, m_server_port);

        const std::string request = "HEAD " + m_path + " HTTP/1.1\r\nHost: " + m_HostHeader +
                                    "\r\nConnection: close\r\n\r\n";

        if (m_Verbose > 1)
        {
            std::cout << "FileHTTPS::GetSize Request: [" << request << "]" << std::endl;
        }
        m_ssl.Write(request.c_str(), (int)request.size());

        std::string initialBody;
        const std::string headers = ReadHTTPHeaders(m_ssl, initialBody, "GetSize");
        const int status = HTTPStatusCode(headers);
        if (status < 200 || status >= 300)
        {
            helper::Throw<std::ios_base::failure>(
                "Toolkit", "transport::file::FileHTTPS", "GetSize",
                "HTTPS HEAD request returned status " + std::to_string(status));
        }
        m_fileSize = HTTPContentLength(headers, "GetSize");
        if (m_BaseOffset > m_fileSize || (m_BaseSize > 0 && m_BaseSize > m_fileSize - m_BaseOffset))
        {
            helper::Throw<std::ios_base::failure>(
                "Toolkit", "transport::file::FileHTTPS", "GetSize",
                "logical byte range is outside the physical HTTPS object " + m_Name);
        }
        if (m_Verbose > 0)
        {
            std::cout << "File size: " << m_fileSize << " bytes\n";
        }

        m_ssl.Close();
        return m_BaseSize > 0 ? m_BaseSize : m_fileSize;
    }
    catch (...)
    {
        m_ssl.Close();
        throw;
    }
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
