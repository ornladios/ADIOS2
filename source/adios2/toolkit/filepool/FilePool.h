

/*
 * FilePool design
 *
 * The idea here is to encapsulate serveral bits of functionality.  First, there's the observation
 * that for some file transports (POSIX), Read() is re-entrant (having no local or kernel state) so
 * the open file can be shared between many threads without difficulty (IFF the transport is
 * reentrant, otherwise not).  Second, the Pool is the entity managing the number of open files to
 * keep it under a limit. Approach: users get a unique_ptr to a PoolableFile via
 * FilePool->acquire();  The PoolableFile contains a shared_ptr to Transport, as well as a link to
 * the PoolEntry (where the m_InUseCount is kept).  The unique_ptr is just for easy of memory
 * tracking as the user can simply let the reference go out of scope and the PoolableFile destructor
 * will return the entry to the pool (decrement the m_InUseCount);
 *
 */

#ifndef ADIOS2_FILEPOOL_H_
#define ADIOS2_FILEPOOL_H_

#include "adios2/helper/adiosCommDummy.h"
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
    PoolableFile(FilePool *pool, PoolEntry *entry)
    : file(entry->m_File), m_Entry(entry), m_OwningPool(pool){};
    ~PoolableFile();
    std::shared_ptr<adios2::Transport> file;
    void Read(char *buffer, size_t size, size_t start = 0);
    PoolEntry *m_Entry;
    FilePool *m_OwningPool;
};

class FilePool
{
public:
    FilePool(adios2::transportman::TransportMan *factory, adios2::Params transportParams,
             size_t OpenFileLimit)
    : m_Factory(factory), m_TransportParams(transportParams), m_OpenFileLimit(OpenFileLimit){};
    // Acquire an object from the pool, creating it if necessary
    std::unique_ptr<PoolableFile> Acquire(const std::string &filename);
    void Release(PoolEntry *obj);
    void SetParameters(const adios2::Params &params);
    std::vector<std::shared_ptr<adios2::Transport>> ListOfTransports();

    // testing only
    size_t GetMax() { return m_MaxFileCount; }

private:
    // The pool's internal storage
    std::mutex PoolMutex;
    adios2::transportman::TransportMan *m_Factory;
    adios2::Params m_TransportParams;
    std::unordered_multimap<std::string, std::shared_ptr<PoolEntry>> m_Pool;
    size_t m_OpenFileLimit;
    size_t m_OpenFileCount = 0;
    bool m_CanShare = false;
    bool m_ShareTestDone = false;
    size_t m_MaxFileCount = 0;
};
#endif /* ADIOS2_FILEPOOL_H_ */
