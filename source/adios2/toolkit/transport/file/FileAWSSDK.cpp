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

#include <algorithm> // sort
#include <cstdio>    // remove
#include <cstring>   // strerror
#include <errno.h>   // errno
#include <fcntl.h>   // open
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

    helper::SetParameterValue("bucket", params, m_BucketPrefix);
    if (m_BucketPrefix.empty())
    {
        if (const char *bucketEnv = std::getenv("ADIOS2_AWS_BUCKET"))
        {
            m_BucketPrefix = std::string(bucketEnv);
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

    helper::SetParameterValue("accesskeyid", params, m_accessKeyID);
    if (m_accessKeyID.empty())
    {
        if (const char *Env = std::getenv("AWS_ACCESS_KEY_ID"))
        {
            m_accessKeyID = std::string(Env);
        }
    }
    helper::SetParameterValue("secretkey", params, m_secretKey);
    if (m_secretKey.empty())
    {
        if (const char *Env = std::getenv("AWS_SECRET_KEY"))
        {
            m_secretKey = std::string(Env);
        }
    }
    helper::SetParameterValue("sessiontoken", params, m_sessionToken);
    if (m_sessionToken.empty())
    {
        if (const char *Env = std::getenv("AWS_SESSION_TOKEN"))
        {
            m_sessionToken = std::string(Env);
        }
    }

    helper::SetParameterValueInt("verbose", params, m_Verbose, "");

    // S3 object mode: "multi" (default) or "single"
    std::string objectModeStr;
    helper::SetParameterValue("s3_object_mode", params, objectModeStr);
    if (!objectModeStr.empty())
    {
        if (objectModeStr == "single")
        {
            m_MultiObjectMode = false;
        }
        else if (objectModeStr == "multi")
        {
            m_MultiObjectMode = true;
        }
        else
        {
            helper::Throw<std::invalid_argument>(
                "Toolkit", "transport::file::FileAWSSDK", "SetParameters",
                "s3_object_mode must be 'multi' or 'single', got '" + objectModeStr + "'");
        }
    }

    std::string recheckStr = "true";
    helper::SetParameterValue("recheck_metadata", params, recheckStr);
    m_RecheckMetadata = helper::StringTo<bool>(recheckStr, "");

    // Part size limits for multipart uploads (accepts human-readable sizes like "64MB", "1GB")
    std::string minPartSizeStr, maxPartSizeStr;
    helper::SetParameterValue("min_part_size", params, minPartSizeStr);
    helper::SetParameterValue("max_part_size", params, maxPartSizeStr);

    if (!minPartSizeStr.empty())
    {
        size_t minPartSize = helper::StringToByteUnits(minPartSizeStr, "min_part_size");
        if (minPartSize < S3_MIN_PART_SIZE)
        {
            helper::Throw<std::invalid_argument>(
                "Toolkit", "transport::file::FileAWSSDK", "SetParameters",
                "min_part_size must be at least 5MB (S3 requirement), got " + minPartSizeStr);
        }
        m_MinPartSize = minPartSize;
    }

    if (!maxPartSizeStr.empty())
    {
        size_t maxPartSize = helper::StringToByteUnits(maxPartSizeStr, "max_part_size");
        if (maxPartSize > S3_MAX_PART_SIZE)
        {
            helper::Throw<std::invalid_argument>(
                "Toolkit", "transport::file::FileAWSSDK", "SetParameters",
                "max_part_size must be at most 5GB (S3 limit), got " + maxPartSizeStr);
        }
        if (maxPartSize < m_MinPartSize)
        {
            helper::Throw<std::invalid_argument>(
                "Toolkit", "transport::file::FileAWSSDK", "SetParameters",
                "max_part_size (" + maxPartSizeStr + ") must be >= min_part_size (" +
                    minPartSizeStr + ")");
        }
        m_MaxPartSize = maxPartSize;
    }

    std::string directThresholdStr;
    helper::SetParameterValue("direct_upload_threshold", params, directThresholdStr);
    if (!directThresholdStr.empty())
    {
        m_DirectUploadThreshold =
            helper::StringToByteUnits(directThresholdStr, "direct_upload_threshold");
    }

    core::ADIOS::Global_init_AWS_API();

    s3ClientConfig = new Aws::S3::S3ClientConfiguration;
    s3ClientConfig->endpointOverride = m_Endpoint;
    s3ClientConfig->useVirtualAddressing = false;
    s3ClientConfig->enableEndpointDiscovery = false;
    s3ClientConfig->region = "us-east-1"; // Required for signature calculation

    if (!m_accessKeyID.empty() || !m_secretKey.empty())
    {
        Aws::Auth::AWSCredentials aws_credentials;
        if (!m_sessionToken.empty())
        {
            aws_credentials = {m_accessKeyID, m_secretKey, m_sessionToken};
        }
        else
        {
            aws_credentials = {m_accessKeyID, m_secretKey};
        }
        s3Client =
            new Aws::S3::S3Client(aws_credentials, *s3ClientConfig,
                                  Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);
    }
    else
    {
        s3Client = new Aws::S3::S3Client(*s3ClientConfig);
    }
    if (m_Verbose > 0)
    {
        std::cout << "FileAWSSDK::SetParameters: AWS Transport created with endpoint = '"
                  << m_Endpoint << "' bucket = '" << m_BucketPrefix
                  << "' object_mode = " << (m_MultiObjectMode ? "multi" : "single")
                  << " recheck_metadata = " << m_RecheckMetadata
                  << " min_part_size = " << m_MinPartSize << " max_part_size = " << m_MaxPartSize
                  << " direct_upload_threshold = " << m_DirectUploadThreshold << std::endl;
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
    // If bucket is specified via parameter, use it directly and name becomes the object key
    if (!m_BucketPrefix.empty())
    {
        m_BucketName = m_BucketPrefix;
        m_ObjectName = name;
        // Strip leading slashes from object key (S3 keys shouldn't start with /)
        // Also strip leading dots, those lead to authentication issues
        while (!m_ObjectName.empty() &&
               (m_ObjectName[0] == PathSeparator || m_ObjectName[0] == '.'))
        {
            m_ObjectName = m_ObjectName.substr(1);
        }
        m_Name = m_BucketName + PathSeparator + m_ObjectName;
    }
    else
    {
        // Original behavior: parse bucket/object from the path
        std::string fullPath = name;
        m_Name = fullPath;

        size_t pos = fullPath.find(PathSeparator);
        if (pos == std::string::npos)
        {
            helper::Throw<std::invalid_argument>("Toolkit", "transport::file::FileAWSSDK", "Open",
                                                 "invalid 'bucket/object' name " + fullPath);
        }
        m_BucketName = fullPath.substr(0, pos);
        m_ObjectName = fullPath.substr(pos + 1);
    }

    m_OpenMode = openMode;
    switch (m_OpenMode)
    {

    case Mode::Write:
        ProfilerStart("open");
        m_WriteBuffer.clear();
        m_TotalBytesWritten = 0;

        if (m_MultiObjectMode)
        {
            // Multi-object mode: save base object name, upload individual objects
            m_BaseObjectName = m_ObjectName;
            m_NextObjectNumber = 0;
        }
        else
        {
            // Single-object mode: one multipart upload per file
            m_WriteBuffer.reserve(m_MinPartSize);
            m_CompletedParts.clear();
            m_CurrentPartNumber = 0;
            InitiateMultipartUpload();
        }

        m_IsOpen = true;
        ProfilerStop("open");
        break;

    case Mode::Append:
        // Append is complex with S3 - objects are immutable after upload
        // Would need to download, modify, re-upload or use multipart copy + new parts
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileAWSSDK", "Open",
                                              "Append mode not supported for S3 " + m_Name);
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
            if (head_object.IsSuccess())
            {
                // Single object exists — use it directly
                m_Size = head_object.GetResult().GetContentLength();
                CheckCache(m_Size);
            }
            else
            {
                // Object not found — try multi-object layout (data.0.0, data.0.1, ...)
                DiscoverSubObjects();
                if (m_IsMultiObjectLayout)
                {
                    m_Size = m_TotalVirtualSize;
                    if (m_Verbose > 0)
                    {
                        std::cout << "FileAWSSDK::Open: Multi-object layout for '" << m_ObjectName
                                  << "': " << m_SubObjects.size()
                                  << " objects, total size = " << m_Size << std::endl;
                    }
                }
                else
                {
                    helper::Throw<std::invalid_argument>(
                        "Toolkit", "transport::file::FileAWSSDK", "Open",
                        "'bucket/object'  " + m_Name + " does not exist ");
                }
            }

            m_Errno = errno;
        }
        ProfilerStop("open");
        break;
    }
    default:
        CheckFile("unknown open mode for file " + m_Name + ", in call to AWSSDK open");
    }

    if (!m_IsOpening && !m_IsOpen)
    {
        // Only check for read mode - write mode already set m_IsOpen = true
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
    WaitForOpen();

    if (m_OpenMode != Mode::Write)
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileAWSSDK", "Write",
                                              "File not opened for writing: " + m_Name);
    }

    // S3 only supports sequential writes
    if (start != MaxSizeT && start != m_TotalBytesWritten)
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileAWSSDK", "Write",
            "S3 only supports sequential writes. "
            "Requested start: " +
                std::to_string(start) +
                ", current position: " + std::to_string(m_TotalBytesWritten) +
                ". Hint: Set StripeSize=0 when using S3 storage.");
    }

    ProfilerStart("write");

    if (m_MultiObjectMode)
    {
        // Multi-object mode: buffer data, with zero-copy for large writes
        if (size >= m_DirectUploadThreshold && !m_WriteBuffer.empty())
        {
            // Large write: flush current buffer as one object, then upload
            // the large buffer directly as another (zero-copy)
            FlushWriteBuffer();
        }

        if (size >= m_DirectUploadThreshold)
        {
            // Upload large data directly without copying into m_WriteBuffer
            std::string key = m_BaseObjectName + "." + std::to_string(m_NextObjectNumber);
            m_NextObjectNumber++;
            UploadObject(key, buffer, size);
        }
        else
        {
            // Small write: accumulate in buffer
            m_WriteBuffer.insert(m_WriteBuffer.end(), buffer, buffer + size);
        }
    }
    else
    {
        // Single-object mode: original multipart upload buffering
        size_t offset = 0;

        // Step 1: If we have buffered data, fill it up to target size first
        if (!m_WriteBuffer.empty())
        {
            size_t spaceInBuffer = m_MinPartSize - m_WriteBuffer.size();
            size_t toBuffer = std::min(size, spaceInBuffer);

            m_WriteBuffer.insert(m_WriteBuffer.end(), buffer, buffer + toBuffer);
            offset = toBuffer;

            if (m_WriteBuffer.size() >= m_MinPartSize)
            {
                UploadBufferedPart();
            }
        }

        // Step 2: Handle remaining data
        size_t remaining = size - offset;

        // Upload large chunks directly (but respect max part size)
        while (remaining >= m_MinPartSize)
        {
            size_t partSize = std::min(remaining, m_MaxPartSize);
            UploadPart(buffer + offset, partSize);
            offset += partSize;
            remaining -= partSize;
        }

        // Buffer any small remainder for later
        if (remaining > 0)
        {
            m_WriteBuffer.insert(m_WriteBuffer.end(), buffer + offset, buffer + size);
        }
    }

    m_TotalBytesWritten += size;
    ProfilerWriteBytes(size);
    ProfilerStop("write");
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

    if (m_IsMultiObjectLayout)
    {
        // Multi-object read: find the sub-object(s) that contain the requested range
        size_t readPos = m_SeekPos;
        size_t bytesRemaining = size;
        size_t bufferOffset = 0;

        while (bytesRemaining > 0)
        {
            // Binary search: find the sub-object containing readPos
            size_t lo = 0, hi = m_SubObjects.size();
            while (lo + 1 < hi)
            {
                size_t mid = (lo + hi) / 2;
                if (m_SubObjects[mid].cumulativeOffset <= readPos)
                {
                    lo = mid;
                }
                else
                {
                    hi = mid;
                }
            }

            const SubObject &sub = m_SubObjects[lo];
            size_t offsetInObject = readPos - sub.cumulativeOffset;
            size_t availableInObject = sub.size - offsetInObject;
            size_t toRead = std::min(bytesRemaining, availableInObject);

            // Fetch range from this sub-object
            Aws::S3::Model::GetObjectRequest request;
            request.SetBucket(m_BucketName);
            request.SetKey(sub.key);
            std::stringstream range;
            range << "bytes=" << offsetInObject << "-" << offsetInObject + toRead - 1;
            request.SetRange(range.str());

            auto outcome = s3Client->GetObject(request);
            if (!outcome.IsSuccess())
            {
                const Aws::S3::S3Error &err = outcome.GetError();
                helper::Throw<std::invalid_argument>(
                    "Toolkit", "transport::file::FileAWSSDK", "Read",
                    "multi-object read failed for '" + sub.key + "' range " + range.str() + ": " +
                        err.GetExceptionName() + ": " + err.GetMessage());
            }

            auto *body = outcome.GetResult().GetBody().rdbuf();
            body->sgetn(buffer + bufferOffset, static_cast<std::streamsize>(toRead));

            readPos += toRead;
            bufferOffset += toRead;
            bytesRemaining -= toRead;
        }
    }
    else
    {
        // Single-object read
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
                std::cout << "FileAWSSDK::Read: Successfully retrieved '" << m_ObjectName
                          << "' from '" << m_BucketName << "'."
                          << "\nObject length = " << outcome.GetResult().GetContentLength()
                          << "\nRange requested = " << range.str() << std::endl;
            }
            auto *body = outcome.GetResult().GetBody().rdbuf();
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
}

size_t FileAWSSDK::GetSize()
{
    WaitForOpen();
    switch (m_OpenMode)
    {
    case Mode::Write:
        return m_TotalBytesWritten;
    case Mode::Append:
        return 0;
    case Mode::Read:
        return m_Size;
    default:
        return 0;
    }
}

void FileAWSSDK::Flush()
{
    if (!m_MultiObjectMode && m_OpenMode == Mode::Write && m_WriteBuffer.size() >= m_MinPartSize)
    {
        // Single mode only: flush if we have enough data for a valid part
        // S3 requires minimum 5MB per part (except the last part)
        UploadBufferedPart();
    }
    // Multi-object mode: Flush is a no-op; use FinalizeSegment() instead
}

void FileAWSSDK::FinalizeSegment()
{
    if (m_OpenMode != Mode::Write)
    {
        return;
    }

    if (m_MultiObjectMode)
    {
        // Upload buffered data as the next numbered object
        FlushWriteBuffer();
    }
    // Single mode: no-op (multipart upload continues across segments)
}

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

    if (m_OpenMode == Mode::Write)
    {
        if (m_MultiObjectMode)
        {
            // Multi-object mode: flush remaining buffered data as final object
            FlushWriteBuffer();
            if (m_Verbose > 0)
            {
                std::cout << "FileAWSSDK::Close: Multi-object mode, wrote " << m_NextObjectNumber
                          << " objects for " << m_Name << std::endl;
            }
        }
        else
        {
            // Single-object mode: finalize multipart upload
            // Upload any remaining data as the final part
            // (Final part can be < 5MB, unlike other parts)
            if (!m_WriteBuffer.empty())
            {
                UploadBufferedPart();
            }

            // Complete or abort the multipart upload
            if (!m_CompletedParts.empty())
            {
                CompleteMultipartUpload();
            }
            else if (!m_UploadId.empty())
            {
                // No parts uploaded (empty file) - abort the upload
                AbortMultipartUpload();
                if (m_Verbose > 0)
                {
                    std::cout << "FileAWSSDK::Close: Aborted empty multipart upload for " << m_Name
                              << std::endl;
                }
            }
        }
    }

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
    if (!m_IsCached && !m_IsMultiObjectLayout && !head_object.IsSuccess())
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

void FileAWSSDK::DiscoverSubObjects()
{
    // List objects with prefix "objectName." to find numbered sub-objects
    // e.g. for "data.0", look for "data.0.0", "data.0.1", ...
    std::string prefix = m_ObjectName + ".";

    m_SubObjects.clear();
    m_TotalVirtualSize = 0;

    Aws::String continuationToken;
    bool moreResults = true;

    while (moreResults)
    {
        Aws::S3::Model::ListObjectsV2Request request;
        request.SetBucket(m_BucketName);
        request.SetPrefix(prefix);
        if (!continuationToken.empty())
        {
            request.SetContinuationToken(continuationToken);
        }

        auto outcome = s3Client->ListObjectsV2(request);
        if (!outcome.IsSuccess())
        {
            if (m_Verbose > 0)
            {
                std::cout << "FileAWSSDK::DiscoverSubObjects: ListObjectsV2 failed for prefix '"
                          << prefix << "': " << outcome.GetError().GetMessage() << std::endl;
            }
            return; // Not multi-object layout
        }

        const auto &result = outcome.GetResult();
        for (const auto &object : result.GetContents())
        {
            std::string key = object.GetKey();
            // Extract the numeric suffix after the prefix
            std::string suffix = key.substr(prefix.length());
            // Verify it's a pure numeric suffix (no nested slashes or other chars)
            bool isNumeric = !suffix.empty();
            for (char c : suffix)
            {
                if (c < '0' || c > '9')
                {
                    isNumeric = false;
                    break;
                }
            }
            if (!isNumeric)
            {
                continue;
            }

            SubObject sub;
            sub.key = key;
            sub.size = static_cast<size_t>(object.GetSize());
            sub.cumulativeOffset = 0; // computed after sorting
            m_SubObjects.push_back(sub);
        }

        if (result.GetIsTruncated())
        {
            continuationToken = result.GetNextContinuationToken();
        }
        else
        {
            moreResults = false;
        }
    }

    if (m_SubObjects.empty())
    {
        return; // No sub-objects found
    }

    // Sort by numeric suffix (object number)
    std::sort(m_SubObjects.begin(), m_SubObjects.end(),
              [&prefix](const SubObject &a, const SubObject &b) {
                  size_t numA = std::stoull(a.key.substr(prefix.length()));
                  size_t numB = std::stoull(b.key.substr(prefix.length()));
                  return numA < numB;
              });

    // Build cumulative offset table
    size_t cumOffset = 0;
    for (auto &sub : m_SubObjects)
    {
        sub.cumulativeOffset = cumOffset;
        cumOffset += sub.size;
    }
    m_TotalVirtualSize = cumOffset;
    m_IsMultiObjectLayout = true;

    if (m_Verbose > 0)
    {
        std::cout << "FileAWSSDK::DiscoverSubObjects: Found " << m_SubObjects.size()
                  << " sub-objects for prefix '" << prefix
                  << "', total size = " << m_TotalVirtualSize << std::endl;
    }
}

void FileAWSSDK::UploadObject(const std::string &key, const char *data, size_t size)
{
    Aws::Utils::Stream::PreallocatedStreamBuf streamBuf(
        reinterpret_cast<unsigned char *>(const_cast<char *>(data)), size);
    auto inputData = Aws::MakeShared<Aws::IOStream>("S3Upload", &streamBuf);

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(m_BucketName);
    request.SetKey(key);
    request.SetBody(inputData);
    request.SetContentLength(static_cast<long long>(size));

    if (m_Verbose > 0)
    {
        std::cout << "FileAWSSDK::UploadObject: bucket='" << m_BucketName << "' key='" << key
                  << "' size=" << size << std::endl;
    }

    auto outcome = s3Client->PutObject(request);
    if (!outcome.IsSuccess())
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileAWSSDK", "UploadObject",
            "Failed to upload object '" + key + "' to bucket '" + m_BucketName +
                "': " + outcome.GetError().GetMessage());
    }
}

void FileAWSSDK::FlushWriteBuffer()
{
    if (m_WriteBuffer.empty())
    {
        return;
    }

    std::string key = m_BaseObjectName + "." + std::to_string(m_NextObjectNumber);
    m_NextObjectNumber++;
    UploadObject(key, m_WriteBuffer.data(), m_WriteBuffer.size());
    m_WriteBuffer.clear();
}

void FileAWSSDK::InitiateMultipartUpload()
{
    Aws::S3::Model::CreateMultipartUploadRequest request;
    request.SetBucket(m_BucketName);
    request.SetKey(m_ObjectName);

    if (m_Verbose > 0)
    {
        std::cout << "FileAWSSDK::InitiateMultipartUpload: bucket='" << m_BucketName << "' object='"
                  << m_ObjectName << "'" << std::endl;
    }

    auto outcome = s3Client->CreateMultipartUpload(request);
    if (!outcome.IsSuccess())
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileAWSSDK",
                                              "InitiateMultipartUpload",
                                              "Failed to initiate multipart upload for " + m_Name +
                                                  ": " + outcome.GetError().GetMessage());
    }

    m_UploadId = outcome.GetResult().GetUploadId();
    if (m_Verbose > 0)
    {
        std::cout << "FileAWSSDK::InitiateMultipartUpload: Upload ID = " << m_UploadId << std::endl;
    }
}

void FileAWSSDK::UploadPart(const char *data, size_t size)
{
    m_CurrentPartNumber++;

    // Zero-copy stream: reads directly from data buffer (no memcpy)
    // Buffer must stay valid until UploadPart returns (synchronous)
    Aws::Utils::Stream::PreallocatedStreamBuf streamBuf(
        reinterpret_cast<unsigned char *>(const_cast<char *>(data)), size);
    auto inputData = Aws::MakeShared<Aws::IOStream>("S3Upload", &streamBuf);

    Aws::S3::Model::UploadPartRequest request;
    request.SetBucket(m_BucketName);
    request.SetKey(m_ObjectName);
    request.SetPartNumber(m_CurrentPartNumber);
    request.SetUploadId(m_UploadId);
    request.SetBody(inputData);
    request.SetContentLength(static_cast<long long>(size));

    if (m_Verbose > 0)
    {
        std::cout << "FileAWSSDK::UploadPart: Uploading part " << m_CurrentPartNumber
                  << ", size=" << size << " bytes" << std::endl;
    }

    auto outcome = s3Client->UploadPart(request);
    if (!outcome.IsSuccess())
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "transport::file::FileAWSSDK", "UploadPart",
            "Failed to upload part " + std::to_string(m_CurrentPartNumber) + " for " + m_Name +
                ": " + outcome.GetError().GetMessage());
    }

    // Record completed part for final CompleteMultipartUpload call
    Aws::S3::Model::CompletedPart completedPart;
    completedPart.SetPartNumber(m_CurrentPartNumber);
    completedPart.SetETag(outcome.GetResult().GetETag());
    m_CompletedParts.push_back(completedPart);

    if (m_Verbose > 0)
    {
        std::cout << "FileAWSSDK::UploadPart: Part " << m_CurrentPartNumber
                  << " uploaded, ETag=" << outcome.GetResult().GetETag() << std::endl;
    }
}

void FileAWSSDK::UploadBufferedPart()
{
    if (m_WriteBuffer.empty())
    {
        return;
    }

    UploadPart(m_WriteBuffer.data(), m_WriteBuffer.size());
    m_WriteBuffer.clear();
}

void FileAWSSDK::CompleteMultipartUpload()
{
    Aws::S3::Model::CompletedMultipartUpload completedUpload;
    completedUpload.SetParts(m_CompletedParts);

    Aws::S3::Model::CompleteMultipartUploadRequest request;
    request.SetBucket(m_BucketName);
    request.SetKey(m_ObjectName);
    request.SetUploadId(m_UploadId);
    request.SetMultipartUpload(completedUpload);

    if (m_Verbose > 0)
    {
        std::cout << "FileAWSSDK::CompleteMultipartUpload: Completing upload for " << m_Name
                  << " with " << m_CompletedParts.size() << " parts" << std::endl;
    }

    auto outcome = s3Client->CompleteMultipartUpload(request);
    if (!outcome.IsSuccess())
    {
        helper::Throw<std::ios_base::failure>("Toolkit", "transport::file::FileAWSSDK",
                                              "CompleteMultipartUpload",
                                              "Failed to complete multipart upload for " + m_Name +
                                                  ": " + outcome.GetError().GetMessage());
    }

    if (m_Verbose > 0)
    {
        std::cout << "FileAWSSDK::CompleteMultipartUpload: Successfully uploaded " << m_Name << ", "
                  << m_CompletedParts.size() << " parts, " << m_TotalBytesWritten << " bytes total"
                  << std::endl;
    }

    m_UploadId.clear();
    m_CompletedParts.clear();
}

void FileAWSSDK::AbortMultipartUpload()
{
    if (m_UploadId.empty())
    {
        return;
    }

    Aws::S3::Model::AbortMultipartUploadRequest request;
    request.SetBucket(m_BucketName);
    request.SetKey(m_ObjectName);
    request.SetUploadId(m_UploadId);

    // Best effort - ignore errors on abort
    auto outcome = s3Client->AbortMultipartUpload(request);
    if (m_Verbose > 0)
    {
        if (outcome.IsSuccess())
        {
            std::cout << "FileAWSSDK::AbortMultipartUpload: Aborted upload for " << m_Name
                      << std::endl;
        }
        else
        {
            std::cout << "FileAWSSDK::AbortMultipartUpload: Failed to abort upload for " << m_Name
                      << ": " << outcome.GetError().GetMessage() << std::endl;
        }
    }

    m_UploadId.clear();
    m_CompletedParts.clear();
}

} // end namespace transport
} // end namespace adios2
