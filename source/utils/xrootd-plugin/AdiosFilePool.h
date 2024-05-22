/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOSFILEPOOL_H_
#define ADIOSFILEPOOL_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm>
#include <cstring>
#include <mutex>
#include <random>
#include <string>
#include <vector>
/// \endcond

#include "adios2.h"
#include "adios2/helper/adiosString.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

using namespace adios2::core;
using namespace adios2;

namespace adios2
{
class AnonADIOSFile
{
public:
    adios2::ADIOS adios;
    adios2::IO m_io;
    adios2::Engine m_engine;
    int64_t m_ID;
    std::string m_IOname;
    std::string m_FileName;
    bool m_RowMajorArrays;
    size_t m_BytesSent = 0;
    size_t m_OperationCount = 0;
    AnonADIOSFile(std::string FileName, bool RowMajorArrays)
    {
        Mode adios_read_mode = adios2::Mode::Read;
        m_FileName = FileName;
        m_IOname = RandomString(8);
        m_RowMajorArrays = RowMajorArrays;
        ArrayOrdering ArrayOrder =
            RowMajorArrays ? ArrayOrdering::RowMajor : ArrayOrdering::ColumnMajor;
        m_io = adios.DeclareIO(m_IOname, ArrayOrder);
        adios_read_mode = adios2::Mode::ReadRandomAccess;
        m_engine = m_io.Open(FileName, adios_read_mode);
        std::memcpy(&m_ID, m_IOname.c_str(), sizeof(m_ID));
    }
    ~AnonADIOSFile()
    {
        m_engine.Close();
        adios.RemoveIO(m_IOname);
    }

private:
    std::string RandomString(const size_t length)
    {
        size_t len = length;
        if (len == 0)
            len = 1;
        if (len > 64)
            len = 64;

        std::string str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzA");

        std::random_device rd;
        std::mt19937 generator(rd());

        std::shuffle(str.begin(), str.end(), generator);

        return str.substr(0, len);
    }
};

class ADIOSFilePool
{

public:
    ADIOSFilePool();
    ~ADIOSFilePool();

    AnonADIOSFile *GetFree(std::string Filename, bool RowMajorArrays);
    void Return(AnonADIOSFile *Entry);
    void FlushUnused();

private:
    class SubPool
    {

    public:
        SubPool() = default;
        ~SubPool() = default;
        std::chrono::steady_clock::time_point last_used;
        size_t in_use_count = 0;
        std::vector<std::unique_ptr<AnonADIOSFile>> m_list;
        std::vector<bool> m_busy;
        AnonADIOSFile *GetFree(std::string Filename, bool RowMajorArrays);
        void Return(AnonADIOSFile *Entry);
    };

    std::mutex pool_mutex;
    std::unordered_map<std::string, std::unique_ptr<SubPool>> map;
};

} // end namespace adios2

#endif /* ADIOSFILEPOOL_H_ */
