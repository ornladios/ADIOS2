/*
 * FilePool
 *
 * The principal use case for the filepool abstraction is to
 * facilitate accessing BP5 data files where 1) we read from many
 * threads concurrently to maximize filesystem bandwidth, 2) there
 * might actually be more data files than we have file descriptors
 * available for use, and 3) there is little predictability in
 * requests.  This implementation takes into account that for some
 * file transports (POSIX), Read() is re-entrant (having no local or
 * kernel state) so the open file transport can be shared between many
 * threads without difficulty (IFF the transport is reentrant,
 * otherwise not).  It also implements TarInfo functionality (directly
 * accessing file elements in an uncompressed tar (or similar) file)
 * and when sharable the same open FD can be used for every subfile
 * contained within the overall Tar.
 *
 * Operation: In the principal data read loop in BP5 we spawn multiple
 * read threads, each of which runs a loop, pulling a read request off
 * of a shared queue, reading that data (from some data file) and
 * moving on to the next request.  Those threads Acquire() a
 * PoolableFile pointer from a FilePool.  Acquire() is protected by a
 * mutex and each acquire operation involves at least an equal_range()
 * operation on the m_Pool unordered_multimap which is indexed by
 * filename.  If there's an available open transport, then the hash
 * and lookup is the principal cost of the operation, however if the
 * file must be Open()'d, then the mutex will be held until that
 * operation is complete.
 *
 * Aquire() returns a unique_ptr to a PoolableFile. The PoolableFile
 * contains a shared_ptr to Transport, as well as a link to the
 * PoolEntry (where the m_InUseCount is kept).  If any BaseOffset or
 * BaseSize results from the TarInfo operations, those are held in the
 * PoolableFile and not in the transport.  The unique_ptr is just for
 * easy of memory tracking as the user can simply let the reference go
 * out of scope and the PoolableFile destructor will return the entry
 * to the pool (I.E. decrement the m_InUseCount, if the underlying
 * PoolEntry is sharable this won't affect any other uses.
 * Sharability is controlled by whether or not the underlying
 * transport has a Reentrant Read routine and isn't set until we
 * create the first transport.)
 *
 * The data file use case in the read loop requires only the Read()
 * operation on poolable files.  This implementation also adds
 * GetSize(), SetParameters() and Close() in order to support access
 * to metadata files.  Metadata files don't fit the "pool" idea
 * particularly well, but using this abstraction means that if an
 * ADIOS file is completely contained within a Tar file, the entire
 * file can be accessed using a single shared transport (if Read is
 * Reentrant in that transport).  GetSize() may return a fixed value
 * (from TarInfo), or use GetSize() from the underlying transport.
 * Close() is included, but merely calls the underlying transport
 * Close().
 *
 */

#ifndef ADIOS2_FILEPOOL_H_
#define ADIOS2_FILEPOOL_H_

#include "adios2/helper/adiosCommDummy.h"
#include "adios2/helper/adiosString.h"
#include "adios2/toolkit/transportman/TransportMan.h"
#include <mutex>

class PoolEntry
{
public:
    PoolEntry(const std::string &name, std::shared_ptr<adios2::Transport> file)
    : m_File(file), m_Name(name)
    {
    }
    ~PoolEntry() {}

    std::shared_ptr<adios2::Transport> m_File;
    size_t m_InUseCount = 0;

private:
    std::string m_Name;
};

class FilePool;

class PoolableFile
{
public:
    PoolableFile(FilePool *pool, PoolEntry *entry, size_t offset, size_t baseSize)
    : file(entry->m_File), m_Entry(entry), m_OwningPool(pool), m_BaseOffset(offset),
      m_BaseSize(baseSize){};
    ~PoolableFile();
    std::shared_ptr<adios2::Transport> file;
    void Read(char *buffer, size_t size, size_t start = 0);
    size_t GetSize();
    void Close();
    void SetParameters(const adios2::Params &p);
    PoolEntry *m_Entry;
    FilePool *m_OwningPool;
    size_t m_BaseOffset; ///< Starting offset in a larger container if exists, usually 0
    size_t m_BaseSize;   ///< Actual size of file in a larger container if exists, usually 0
};

class FilePool
{
public:
    FilePool(adios2::transportman::TransportMan *factory, adios2::Params transportParams,
             size_t OpenFileLimit, adios2::helper::TarInfoMap *TarInfoMap)
    : m_Factory(factory), m_TransportParams(transportParams), m_TarInfoMap(TarInfoMap),
      m_OpenFileLimit(OpenFileLimit){};
    // Acquire a Poolablefile object from the pool, creating it if necessary
    std::unique_ptr<PoolableFile> Acquire(const std::string &filename,
                                          const bool skipTarInfo = false);
    void Release(PoolEntry *obj);
    // Remove all unused pool entries for a given filename, closing their transports.
    void Evict(const std::string &filename);
    void SetParameters(const adios2::Params &params);
    std::vector<std::shared_ptr<adios2::Transport>> ListOfTransports();

    // testing only
    size_t GetMax() { return m_MaxFileCount; }

private:
    // The pool's internal storage
    std::mutex PoolMutex;
    adios2::transportman::TransportMan *m_Factory;
    adios2::Params m_TransportParams;
    adios2::helper::TarInfoMap *m_TarInfoMap;
    std::unordered_multimap<std::string, std::shared_ptr<PoolEntry>> m_Pool;
    size_t m_OpenFileLimit;
    size_t m_OpenFileCount = 0;
    bool m_CanShare = false;
    bool m_ShareTestDone = false;
    size_t m_MaxFileCount = 0;
};
#endif /* ADIOS2_FILEPOOL_H_ */
