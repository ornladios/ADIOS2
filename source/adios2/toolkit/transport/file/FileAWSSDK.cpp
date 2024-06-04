/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDescriptor.h wrapper of AWSSDK library functions for file I/O over S3
 * protocol
 *
 *  Created on: Dec 5, 2022
 *      Author: Norbert Podhorszki, pnorbert@ornl.gov
 */
#include "FileAWSSDK.h"
#include "adios2/core/ADIOS.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosString.h"
#include "adios2/helper/adiosSystem.h"

#include <cstdio>  // remove
#include <cstring> // strerror
#include <errno.h> // errno
#include <fcntl.h> // open
#include <regex>
#include <sys/stat.h>  // open, fstat
#include <sys/types.h> // open
#include <unistd.h>    // write, close, ftruncate

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

namespace adios2
{
namespace transport
{

FileAWSSDK::FileAWSSDK(helper::Comm const &comm)
: Transport("File", "AWSSDK", comm) /*, m_Impl(&m_ImplSingleton)*/
{
}

FileAWSSDK::~FileAWSSDK()
{
    if (m_IsOpen)
    {
        Close();
    }
}

void FileAWSSDK::SetParameters(const Params &params)
{
    // Parameters are set from config parameters if present
    // Otherwise, they are set from environment if present
    // Otherwise, they remain at their default value

    helper::SetParameterValue("endpoint", params, m_Endpoint);
    if (m_Endpoint.empty())
    {
        if (const char *epEnv = std::getenv("AWS_ENDPOINT"))
        {
            m_Endpoint = std::string(epEnv);
        }
    }

    helper::SetParameterValue("cache", params, m_CachePath);
    if (m_CachePath.empty())
    {
        if (const char *Env = std::getenv("AWS_CACHE"))
        {
            m_CachePath = std::string(Env);
        }
    }

    helper::SetParameterValueInt("verbose", params, m_Verbose, "");

    std::string recheckStr = "true";
    helper::SetParameterValue("recheck_metadata", params, recheckStr);
    m_RecheckMetadata = helper::StringTo<bool>(recheckStr, "");

    core::ADIOS::Global_init_AWS_API();

    s3ClientConfig = new Aws::S3::S3ClientConfiguration;
    s3ClientConfig->endpointOverride = m_Endpoint;
    s3ClientConfig->useVirtualAddressing = false;
    s3ClientConfig->enableEndpointDiscovery = false;

    s3Client = new Aws::S3::S3Client(*s3ClientConfig);
    if (m_Verbose > 0)
    {
        std::cout << "FileAWSSDK::SetParameters: AWS Transport created with endpoint = '"
                  << m_Endpoint << "'"
                  << " recheck_metadata = " << m_RecheckMetadata << std::endl;
    }
}

void FileAWSSDK::WaitForOpen()
{
    if (m_IsOpening)
    {
        if (m_OpenFuture.valid())
        {
            // m_FileDescriptor = m_OpenFuture.get();
        }
        m_IsOpening = false;
        CheckFile("couldn't open file " + m_Name + ", in call to AWSSDK open");
        m_IsOpen = true;
    }
}

void FileAWSSDK::SetUpCache()
{
    if (!m_CachePath.empty())
    {
        if (helper::EndsWith(m_ObjectName, "md.idx") || helper::EndsWith(m_ObjectName, "md.0") ||
            helper::EndsWith(m_ObjectName, "mmd.0"))
        {
            m_CachingThisFile = true;
        }
    }

    if (m_CachingThisFile)
    {
        std::string const ep = std::regex_replace(m_Endpoint, std::regex("/|:"), "_");
        const std::string path(m_CachePath + PathSeparator + ep + PathSeparator + m_BucketName +
                               PathSeparator + m_ObjectName);
        m_CacheFilePath = path;
        if (!m_RecheckMetadata)
        {
            m_CacheFileRead = new FileFStream(m_Comm);
            try
            {
                m_CacheFileRead->Open(path, Mode::Read);
                m_Size = m_CacheFileRead->GetSize();
                m_IsCached = true;
                m_CachingThisFile = false;
                if (m_Verbose > 0)
                {
                    std::cout << "FileAWSSDK::SetUpCache: Already cached " << path
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

void FileAWSSDK::CheckCache(const size_t fileSize)
{
    if (m_CachingThisFile)
    {
        /* Check if cache file exists and size equals the cloud version*/
        m_CacheFileRead = new FileFStream(m_Comm);
        try
        {
            m_CacheFileRead->Open(m_CacheFilePath, Mode::Read);
            size_t cacheSize = m_CacheFileRead->GetSize();
            if (cacheSize == fileSize)
            {
                m_IsCached = true;
                m_CachingThisFile = false;
                if (m_Verbose > 0)
                {
                    std::cout << "FileAWSSDK::CheckCache: Already cached " << m_CacheFilePath
                              << ", full size = " << cacheSize << std::endl;
                }
            }
            else
            {
                std::cout << "FileAWSSDK::CheckCache: Partially cached " << m_CacheFilePath
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
                helper::CreateDirectory(dirpath);
            }
            m_CacheFileWrite = new FileFStream(m_Comm);
            m_CacheFileWrite->Open(m_CacheFilePath, Mode::Write);
            if (m_Verbose > 0)
            {
                std::cout << "FileAWSSDK::CheckCache: Caching turn on for " << m_CacheFilePath
                          << std::endl;
            }
        }
    }
}

void FileAWSSDK::Open(const std::string &name, const Mode openMode, const bool async,
                      const bool directio)
{
    m_Name = name;

    size_t pos = name.find(PathSeparator);
    if (pos == std::string::npos)
    {
        helper::Throw<std::invalid_argument>("Toolkit", "transport::file::FileAWSSDK", "Open",
                                             "invalid 'bucket/object' name " + name);
    }
    m_BucketName = name.substr(0, pos);
    m_ObjectName = name.substr(pos + 1);

    m_OpenMode = openMode;
    switch (m_OpenMode)
    {

    case Mode::Write:
    case Mode::Append:
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileAWSSDK", "Open",
                                              "does not support writing yet " + m_Name);
        break;

    case Mode::Read: {
        ProfilerStart("open");
        errno = 0;

        SetUpCache();
        // m_IsCached=true if already found in cache and m_RecheckMetadata=false
        // m_CachingThisFile=true if we want caching and m_IsCached=false
        // m_CacheFilePath is the path to the local file in cache

        if (!m_IsCached)
        {
            Aws::S3::Model::HeadObjectRequest head_object_request;
            head_object_request.SetBucket(m_BucketName);
            head_object_request.SetKey(m_ObjectName);

            if (m_Verbose > 0)
            {
                std::cout << "FileAWSSDK::Open: S3 HeadObjectRequests bucket='"
                          << head_object_request.GetBucket() << "'  object = '"
                          << head_object_request.GetKey() << "'" << std::endl;
            }
            head_object = s3Client->HeadObject(head_object_request);
            if (!head_object.IsSuccess())
            {
                helper::Throw<std::invalid_argument>(
                    "Toolkit", "transport::file::FileAWSSDK", "Open",
                    "'bucket/object'  " + m_Name + " does not exist ");
            }
            else
            {
                m_Size = head_object.GetResult().GetContentLength();
                /* Cache: check if we want to cache this file (metadata files)
                   and if we already have it fully in the cache
                */
                CheckCache(m_Size);
            }

            m_Errno = errno;
        }
        ProfilerStop("open");
        break;
    }
    default:
        CheckFile("unknown open mode for file " + m_Name + ", in call to AWSSDK open");
    }

    if (!m_IsOpening)
    {
        CheckFile("couldn't open file " + m_Name + ", in call to AWSSDK open");
        m_IsOpen = true;
    }
}

void FileAWSSDK::OpenChain(const std::string &name, Mode openMode, const helper::Comm &chainComm,
                           const bool async, const bool directio)
{
    int token = 1;
    if (chainComm.Rank() > 0)
    {
        chainComm.Recv(&token, 1, chainComm.Rank() - 1, 0, "Chain token in FileAWSSDK::OpenChain");
    }

    Open(name, openMode, async, directio);

    if (chainComm.Rank() < chainComm.Size() - 1)
    {
        chainComm.Isend(&token, 1, chainComm.Rank() + 1, 0,
                        "Sending Chain token in FileAWSSDK::OpenChain");
    }
}

void FileAWSSDK::Write(const char *buffer, size_t size, size_t start)
{
    helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileAWSSDK", "Write",
                                          "does not support writing yet " + m_Name);
}

void FileAWSSDK::Read(char *buffer, size_t size, size_t start)
{
    WaitForOpen();

    if (start != MaxSizeT)
    {
        if (start >= m_Size)
        {
            helper::Throw<std::ios_base::failure>(
                "Toolkit", "transport::file::FileAWSSDK", "Read",
                "couldn't move to start position " + std::to_string(start) +
                    " beyond the size of " + m_Name + " which is " + std::to_string(m_Size));
        }
        m_SeekPos = start;
        errno = 0;
        m_Errno = errno;
    }

    if (m_SeekPos + size > m_Size)
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileAWSSDK", "Read",
                                              "can't read " + std::to_string(size) +
                                                  " bytes from position " +
                                                  std::to_string(m_SeekPos) + " from " + m_Name +
                                                  " whose size is " + std::to_string(m_Size));
    }

    if (m_IsCached)
    {
        m_CacheFileRead->Read(buffer, size, m_SeekPos);
        if (m_Verbose > 0)
        {
            std::cout << "FileAWSSDK::Read: Read from cache " << m_CacheFileRead->m_Name
                      << " start = " << m_SeekPos << " size = " << size << std::endl;
        }
        return;
    }

    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(m_BucketName);
    request.SetKey(m_ObjectName);
    std::stringstream range;
    range << "bytes=" << m_SeekPos << "-" << m_SeekPos + size - 1;
    request.SetRange(range.str());

    Aws::S3::Model::GetObjectOutcome outcome = s3Client->GetObject(request);

    if (!outcome.IsSuccess())
    {
        const Aws::S3::S3Error &err = outcome.GetError();
        helper::Throw<std::invalid_argument>(
            "Toolkit", "transport::file::FileAWSSDK", "Read",
            "'bucket/object'  " + m_Name + ", range " + range.str() +
                "GetObject: " + err.GetExceptionName() + ": " + err.GetMessage());
    }
    else
    {
        if (m_Verbose > 0)
        {
            std::cout << "FileAWSSDK::Read: Successfully retrieved '" << m_ObjectName << "' from '"
                      << m_BucketName << "'."
                      << "\nObject length = " << outcome.GetResult().GetContentLength()
                      << "\nRange requested = " << range.str() << std::endl;
        }
        auto body = outcome.GetResult().GetBody().rdbuf();
        body->sgetn(buffer, size);

        /* Save to cache */
        if (m_CachingThisFile)
        {
            m_CacheFileWrite->Write(buffer, size, m_SeekPos);
            m_CacheFileWrite->Flush();
            if (m_Verbose > 0)
            {
                std::cout << "FileAWSSDK::Read: Written to cache " << m_CacheFileWrite->m_Name
                          << " start = " << m_SeekPos << " size = " << size << std::endl;
            }
        }
    }
}

size_t FileAWSSDK::GetSize()
{
    WaitForOpen();
    switch (m_OpenMode)
    {
    case Mode::Write:
    case Mode::Append:
        return 0;
        break;
    case Mode::Read:
        return m_Size;
        break;
    default:
        return 0;
    }
}

void FileAWSSDK::Flush() {}

void FileAWSSDK::Close()
{
    WaitForOpen();
    if (m_Verbose > 0)
    {
        std::cout << "FileAWSSDK::Close(" << m_Name << ") Enter" << std::endl;
    }
    ProfilerStart("close");
    errno = 0;
    m_Errno = errno;
    if (s3Client)
    {
        delete s3Client;
        s3Client = nullptr;
    }
    if (s3ClientConfig)
    {
        delete s3ClientConfig;
        s3ClientConfig = nullptr;
    }
    if (m_CachingThisFile)
    {
        m_CacheFileWrite->Close();
        delete m_CacheFileWrite;
    }
    if (m_IsCached)
    {
        m_CacheFileRead->Close();
        delete m_CacheFileRead;
    }

    m_IsOpen = false;
    ProfilerStop("close");
}

void FileAWSSDK::Delete()
{
    WaitForOpen();
    if (m_IsOpen)
    {
        Close();
    }
    std::remove(m_Name.c_str());
}

void FileAWSSDK::CheckFile(const std::string hint) const
{
    if (!m_IsCached && !head_object.IsSuccess())
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileAWSSDK", "CheckFile",
                                              hint);
    }
}

void FileAWSSDK::SeekToEnd() { m_SeekPos = MaxSizeT; }

void FileAWSSDK::SeekToBegin() { m_SeekPos = 0; }

void FileAWSSDK::Seek(const size_t start)
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

void FileAWSSDK::Truncate(const size_t length)
{
    helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileAWSSDK", "Truncate",
                                          "does not support truncating " + m_Name);
}

void FileAWSSDK::MkDir(const std::string &fileName) {}

} // end namespace transport
} // end namespace adios2
