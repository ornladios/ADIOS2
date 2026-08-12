/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adiosSystem.h"
#include <chrono> //system_clock, now
#include <cstdlib> // getenv, setenv/unsetenv or _dupenv_s/_putenv_s
#include <ctime>
#include <filesystem>
#include <fstream>
#include <stdexcept> // std::runtime_error, std::exception
#include <system_error>
#include <thread>

#ifndef _WIN32
#include <sys/resource.h> // getrlimits, setrlimits
#include <sys/time.h>
#endif

#include <unordered_set>

#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosString.h"

// needed by IsHDF5File()
#include "adios2/core/IO.h"
#include "adios2/toolkit/transportman/TransportMan.h"
#include <cstring>

// remove ctime warning on Windows
#ifdef _WIN32
#pragma warning(disable : 4996) // ctime warning
#endif

namespace adios2
{
namespace helper
{

bool CreateDirectory(const std::string &fullPath) noexcept
{
    std::error_code ec;
    std::filesystem::create_directories(fullPath, ec);
    return !ec;
}

bool IsLittleEndian() noexcept
{
    uint16_t hexa = 0x1234;
    return *reinterpret_cast<uint8_t *>(&hexa) != 0x12; // NOLINT
}

std::string LocalTimeDate() noexcept
{
    struct tm now_tm;
    char buf[30];

    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

#ifdef _WIN32
    localtime_s(&now_tm, &now);
#else
    localtime_r(&now, &now_tm);
#endif
    strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y\n", &now_tm);

    return std::string(buf);
}

bool IsRowMajor(const std::string hostLanguage) noexcept
{
    bool isRowMajor = true;

    if (hostLanguage == "Fortran" || hostLanguage == "R" || hostLanguage == "Matlab")
    {
        isRowMajor = false;
    }

    return isRowMajor;
}

bool IsZeroIndexed(const std::string hostLanguage) noexcept
{
    bool isZeroIndexed = true;

    if (hostLanguage == "Fortran" || hostLanguage == "R")
    {
        isZeroIndexed = false;
    }
    return isZeroIndexed;
}

int ExceptionToError(const std::string &function)
{
    try
    {
        throw;
    }
    catch (std::invalid_argument &e)
    {
        helper::Log("Helper", "adiosSystem", "ExceptionToError", function + ": " + e.what(),
                    helper::FATALERROR);
        return 1;
    }
    catch (std::system_error &e)
    {
        helper::Log("Helper", "adiosSystem", "ExceptionToError", function + ": " + e.what(),
                    helper::FATALERROR);
        return 2;
    }
    catch (std::runtime_error &e)
    {
        helper::Log("Helper", "adiosSystem", "ExceptionToError", function + ": " + e.what(),
                    helper::FATALERROR);
        return 3;
    }
    catch (std::exception &e)
    {
        helper::Log("Helper", "adiosSystem", "ExceptionToError", function + ": " + e.what(),
                    helper::FATALERROR);
        return 4;
    }
}

bool IsHDF5FileLocal(const std::string &name, core::IO &io, helper::Comm &comm,
                     const std::vector<Params> &transportsParameters) noexcept
{
    bool isHDF5 = false;
    try
    {
        transportman::TransportMan tm(io, comm);
        if (transportsParameters.empty())
        {
            std::vector<Params> defaultTransportParameters(1);
            defaultTransportParameters[0]["transport"] = "File";
            tm.OpenFiles({name}, adios2::Mode::Read, defaultTransportParameters, false);
        }
        else
        {
            tm.OpenFiles({name}, adios2::Mode::Read, transportsParameters, false);
        }
        const unsigned char HDF5Header[8] = {137, 72, 68, 70, 13, 10, 26, 10};
        if (tm.GetFileSize(0) >= 8)
        {
            char header[8];
            tm.ReadFile(header, 8, 0);
            tm.CloseFiles();
            isHDF5 = !std::memcmp(header, HDF5Header, 8);
        }
    }
    catch (std::ios_base::failure &)
    {
        isHDF5 = false;
    }
    return isHDF5;
}

bool IsHDF5File(const std::string &name, core::IO &io, helper::Comm &comm,
                const std::vector<Params> &transportsParameters) noexcept
{
    size_t flag = 0;
    if (!comm.Rank())
    {
        flag = IsHDF5FileLocal(name, io, comm, transportsParameters) ? 1 : 0;
    }
    flag = comm.BroadcastValue(flag);
    return (flag == 1);
}

char BPVersionLocal(const std::string &name) noexcept
{
    // Read the BP version byte at a fixed offset in md.idx (same for BP4/BP5);
    // an absent or short index reads as the current format, BP5. Content-based,
    // so it does not race a concurrent writer's file creation.
    constexpr std::streamoff versionOffset = 37; // BP4Base/BP5Engine m_BPVersionPosition
    char version = '5';
    std::ifstream idx(name + PathSeparator + "md.idx", std::ios::binary);
    char v = 0;
    if (idx && idx.seekg(versionOffset).read(&v, 1) && v >= 3 && v <= 5)
    {
        version = static_cast<char>('0' + v);
    }
    return version;
}

bool IsDAOSDataset(const std::string &name) noexcept
{
    // The DAOS engine writes a data_oids.txt index alongside its
    // metadata files; BP3/BP4/BP5 never produce this file, so it is the
    // unambiguous marker for a DAOS-engine dataset directory.
    std::error_code ec;
    return std::filesystem::exists(
        std::filesystem::symlink_status(name + PathSeparator + "data_oids.txt", ec));
}

unsigned int NumHardwareThreadsPerNode() { return std::thread::hardware_concurrency(); }

size_t RaiseLimitNoFile()
{
#ifdef _WIN32
    return _setmaxstdio(8192);
#else
    static size_t raisedLimit = 0;
    static bool firstCallRaiseLimit = true;

    if (firstCallRaiseLimit)
    {
        struct rlimit limit;
        errno = 0;
        int err = getrlimit(RLIMIT_NOFILE, &limit);
        raisedLimit = limit.rlim_cur;
        if (!err)
        {
            if (limit.rlim_cur < limit.rlim_max)
            {
                limit.rlim_cur = limit.rlim_max;
                err = setrlimit(RLIMIT_NOFILE, &limit);
                if (!err)
                {
                    getrlimit(RLIMIT_NOFILE, &limit);
                    raisedLimit = limit.rlim_cur;
                }
            }
        }

        if (err)
        {
            std::cerr << "adios2::helper::RaiseLimitNoFile(soft=" << limit.rlim_cur
                      << ", hard=" << limit.rlim_max << ") failed with error code " << errno << ": "
                      << strerror(errno) << std::endl;
        }

        firstCallRaiseLimit = false;
    }
    return raisedLimit;
#endif
}

void CleanupBPDirectory(const std::string &directory, const std::vector<std::string> &filesToKeep,
                        helper::Comm &comm)
{
    if (!comm.Rank())
    {
        // Build a set of basenames for O(1) lookup
        std::unordered_set<std::string> keepSet;
        for (const auto &fullPath : filesToKeep)
        {
            std::string basename = std::filesystem::path(fullPath).filename().string();
            keepSet.insert(basename);
        }

        // Scan directory and delete files not in whitelist
        std::error_code ec;
        for (const auto &entry : std::filesystem::directory_iterator(directory, ec))
        {
            const std::string fileName = entry.path().filename().string();
            // Delete if not in whitelist
            if (keepSet.find(fileName) == keepSet.end())
            {
                std::error_code removeEc;
                if (entry.is_directory(removeEc))
                {
                    std::filesystem::remove_all(entry.path(), removeEc);
                }
                else
                {
                    std::filesystem::remove(entry.path(), removeEc);
                }
            }
        }
    }
    comm.Barrier("CleanupBPDirectory");
}

bool GetEnv(const std::string &key, std::string &result) noexcept
{
#ifdef _WIN32
    char *buffer = nullptr;
    size_t size = 0;
    if (_dupenv_s(&buffer, &size, key.c_str()) != 0 || buffer == nullptr)
    {
        return false;
    }
    result = buffer;
    free(buffer);
    return true;
#else
    const char *value = std::getenv(key.c_str());
    if (!value)
    {
        return false;
    }
    result = value;
    return true;
#endif
}

bool PutEnv(const std::string &env) noexcept
{
    const auto pos = env.find('=');
#ifdef _WIN32
    if (pos == std::string::npos)
    {
        return _putenv_s(env.c_str(), "") == 0;
    }
    return _putenv_s(env.substr(0, pos).c_str(), env.substr(pos + 1).c_str()) == 0;
#else
    if (pos == std::string::npos)
    {
        return unsetenv(env.c_str()) == 0;
    }
    return setenv(env.substr(0, pos).c_str(), env.substr(pos + 1).c_str(), 1) == 0;
#endif
}

std::vector<std::string> SplitString(const std::string &input, char separator, bool isPath)
{
    std::string path = input;
    std::vector<std::string> paths;
    if (path.empty())
    {
        return paths;
    }
    if (isPath && path.front() == '/')
    {
        path.erase(path.begin());
        paths.emplace_back("/");
    }
    std::string::size_type pos1 = 0;
    std::string::size_type pos2 = path.find(separator, pos1);
    while (pos2 != std::string::npos)
    {
        paths.push_back(path.substr(pos1, pos2 - pos1));
        pos1 = pos2 + 1;
        pos2 = path.find(separator, pos1 + 1);
    }
    paths.push_back(path.substr(pos1, pos2 - pos1));
    return paths;
}

} // end namespace helper
} // end namespace adios2
