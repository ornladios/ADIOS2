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

#ifndef ADIOS2_TOOLKIT_TRANSPORT_FILE_AWSSDK_H_
#define ADIOS2_TOOLKIT_TRANSPORT_FILE_AWSSDK_H_

#include <future> //std::async, std::future

#include "adios2/common/ADIOSConfig.h"
#include "adios2/toolkit/transport/Transport.h"
#include "adios2/toolkit/transport/file/FileFStream.h"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/AbortMultipartUploadRequest.h>
#include <aws/s3/model/CompleteMultipartUploadRequest.h>
#include <aws/s3/model/CompletedMultipartUpload.h>
#include <aws/s3/model/CompletedPart.h>
#include <aws/s3/model/CreateMultipartUploadRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/Delete.h>
#include <aws/s3/model/DeleteObjectsRequest.h>
#include <aws/s3/model/ObjectIdentifier.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/UploadPartRequest.h>

namespace adios2
{
namespace helper
{
class Comm;
}
namespace transport
{

/** File descriptor transport using the AWSSDK IO library */
class FileAWSSDK : public Transport
{

public:
    FileAWSSDK(helper::Comm const &comm);

    ~FileAWSSDK();

    void SetParameters(const Params &parameters);

    void Open(const std::string &name, const Mode openMode, const bool async = false,
              const bool directio = false) final;

    void OpenChain(const std::string &name, Mode openMode, const helper::Comm &chainComm,
                   const bool async = false, const bool directio = false) final;

    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final;

#ifdef REALLY_WANT_WRITEV
    /* Actual writev() function, inactive for now */
    void WriteV(const core::iovec *iov, const int iovcnt, size_t start = MaxSizeT) final;
#endif

    void Read(char *buffer, size_t size, size_t start = 0) final;

    size_t GetSize() final;

    /** Does nothing, each write is supposed to flush */
    void Flush() final;

    void FinalizeSegment() final;

    void Close() final;

    void Delete() final;

    void SeekToEnd() final;

    void SeekToBegin() final;

    void Seek(const size_t start = MaxSizeT) final;

    size_t CurrentPos() final { return m_SeekPos; };

    void Truncate(const size_t length) final;

    void MkDir(const std::string &fileName) final;

private:
    // class Impl;
    // static class Impl m_ImplSingleton;
    // Impl *m_Impl;
    // std::unique_ptr<Impl> m_Impl;
    Aws::S3::S3ClientConfiguration *s3ClientConfig = nullptr;
    Aws::S3::S3Client *s3Client = nullptr;
    /** AWSSDK file handle returned by Open */
    std::string m_Endpoint;
    std::string m_BucketPrefix; // Bucket to prepend to paths (from "bucket" parameter)
    std::string m_accessKeyID;
    std::string m_secretKey;
    std::string m_sessionToken;
    Aws::S3::Model::HeadObjectOutcome head_object;
    std::string m_BucketName;
    std::string m_ObjectName;
    int m_Errno = 0;
    bool m_IsOpening = false;
    std::future<int> m_OpenFuture;
    size_t m_SeekPos = 0;
    size_t m_Size = 0;
    int m_Verbose = 0;
    bool m_RecheckMetadata = true; // always check if cached metadata is complete

    void SetUpCache();
    void CheckCache(const size_t fileSize);
    std::string m_CachePath;        // local cache directory
    bool m_CachingThisFile = false; // save content to local cache
    FileFStream *m_CacheFileWrite;
    bool m_IsCached = false; // true if file is already in cache
    FileFStream *m_CacheFileRead;
    std::string m_CacheFilePath; // full path to file in cache
    std::string m_FileNameInTar; // original file name if we work inside a TAR file

    // Object mode: "multi" (default) = one PutObject per segment,
    //              "single" = one multipart upload per file
    bool m_MultiObjectMode = true;

    // Multi-object mode state
    std::string m_BaseObjectName;  // e.g. "data.0" (object name before numbering)
    size_t m_NextObjectNumber = 0; // sequential counter for numbered objects
    size_t m_DirectUploadThreshold = 1ULL * 1024 * 1024; // 1MB - large writes bypass buffer

    // Multi-object mode helpers
    void UploadObject(const std::string &key, const char *data, size_t size);
    void FlushWriteBuffer(); // upload m_WriteBuffer as next numbered object
    void DeleteStaleObjects(); // remove all data.* objects from a previous write

    // Multipart upload state for single-object mode
    std::string m_UploadId;
    std::vector<Aws::S3::Model::CompletedPart> m_CompletedParts;
    int m_CurrentPartNumber = 0;

    // Write buffer - used by both modes
    // Single mode: accumulate at least S3_MIN_PART_SIZE before uploading part
    // Multi mode: accumulate data between FinalizeSegment() calls
    static constexpr size_t S3_MIN_PART_SIZE = 5ULL * 1024 * 1024;        // 5 MiB (S3 limit)
    static constexpr size_t S3_MAX_PART_SIZE = 5ULL * 1024 * 1024 * 1024; // 5 GiB (S3 limit)
    size_t m_MinPartSize = S3_MIN_PART_SIZE; // configurable via min_part_size (must be >= 5 MiB)
    size_t m_MaxPartSize = S3_MAX_PART_SIZE; // configurable via max_part_size (must be <= 5 GiB)
    std::vector<char> m_WriteBuffer;
    size_t m_TotalBytesWritten = 0;

    // Multipart upload helper methods (single-object mode only)
    void InitiateMultipartUpload();
    void UploadPart(const char *data, size_t size);
    void UploadBufferedPart();
    void CompleteMultipartUpload();
    void AbortMultipartUpload();

    // Multi-object read state: virtual concatenated file
    struct SubObject
    {
        std::string key;
        size_t size;
        size_t cumulativeOffset; // start offset of this object in virtual file
    };
    std::vector<SubObject> m_SubObjects;
    size_t m_TotalVirtualSize = 0;
    bool m_IsMultiObjectLayout = false;

    void DiscoverSubObjects();

    /**
     * Check if m_FileDescriptor is -1 after an operation
     * @param hint exception message
     */
    void CheckFile(const std::string hint) const;
    void WaitForOpen();
};

} // end namespace transport
} // end namespace adios2

#endif /* ADIOS2_TRANSPORT_FILE_AWSSDK_H_ */
