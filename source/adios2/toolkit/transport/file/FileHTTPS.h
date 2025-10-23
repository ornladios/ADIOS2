#ifndef ADIOS2_FILEHTTPS_H
#define ADIOS2_FILEHTTPS_H

#include "../Transport.h"
#include "./FileFStream.h"
#include "adios2/common/ADIOSConfig.h"
#include "adios2/helper/adiosNetwork.h" // NetworkSocket

namespace adios2
{
namespace helper
{
class Comm;
}
namespace transport
{

/** File descriptor transport using the POSIX IO library over HTTPS */
class FileHTTPS : public Transport
{

public:
    FileHTTPS(helper::Comm const &comm);

    ~FileHTTPS();

    void SetParameters(const Params &parameters);

    void Open(const std::string &name, const Mode openMode, const bool async = false,
              const bool directio = false) final;

    void OpenChain(const std::string &name, Mode openMode, const helper::Comm &chainComm,
                   const bool async = false, const bool directio = false) final;

    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final;

    void Read(char *buffer, size_t size, size_t start = MaxSizeT) final;

    size_t GetSize() final;

    void Flush() final{};

    void Close() final{};

    void Delete() final{};

    void SeekToEnd() final;

    void SeekToBegin() final;

    void Seek(const size_t start = MaxSizeT) final;

    size_t CurrentPos() final { return 0; };

    void Truncate(const size_t length) final;

    void MkDir(const std::string &fileName) final;

private:
    std::string m_hostname, m_path;
    uint16_t m_server_port = 443; // HTTPS default
    helper::SSLSocket m_ssl;

    int m_Errno = 0;
    bool m_IsOpening = false;

    std::string request_template = "GET %s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%d-%d\r\n\r\n";

    size_t m_fileSize = 0;

    void CheckFile(const std::string hint) const;
    void WaitForOpen();
    std::string SysErrMsg() const;

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
};

} // end namespace transport
} // end namespace adios2

#endif // ADIOS2_FILEHTTPS_H
