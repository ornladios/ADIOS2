/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Test that BP5Reader closes metadata file descriptors (md.idx, md.0, mmd.0)
 * after opening a completed file in ReadRandomAccess mode.
 *
 * Uses /proc/self/fd on Linux and fcntl(F_GETPATH) on macOS to inspect
 * which files the process has open.
 */

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../TestHelpers.h"

#if defined(__linux__)
#include <dirent.h>
#include <unistd.h>
#define HAS_FD_INSPECTION 1
#elif defined(__APPLE__)
#include <fcntl.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <unistd.h>
#define HAS_FD_INSPECTION 1
#else
#define HAS_FD_INSPECTION 0
#endif

std::string engineName;       // comes from command line
std::string engineParameters; // comes from command line

#if HAS_FD_INSPECTION
/* Return the number of open file descriptors whose path contains `substr`. */
static int CountOpenFDs(const std::string &substr)
{
    int count = 0;

#if defined(__linux__)
    DIR *dir = opendir("/proc/self/fd");
    if (!dir)
    {
        return -1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }
        char linkTarget[PATH_MAX];
        std::string fdPath = std::string("/proc/self/fd/") + entry->d_name;
        ssize_t len = readlink(fdPath.c_str(), linkTarget, sizeof(linkTarget) - 1);
        if (len > 0)
        {
            linkTarget[len] = '\0';
            if (std::string(linkTarget).find(substr) != std::string::npos)
            {
                ++count;
            }
        }
    }
    closedir(dir);

#elif defined(__APPLE__)
    /* On macOS, iterate FD numbers and use fcntl(F_GETPATH) to get the path. */
    struct rlimit rl;
    rlim_t maxfd = 256;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0 && rl.rlim_cur < 4096)
    {
        maxfd = rl.rlim_cur;
    }
    char pathbuf[MAXPATHLEN];
    for (int fd = 0; fd < (int)maxfd; ++fd)
    {
        if (fcntl(fd, F_GETPATH, pathbuf) != -1)
        {
            if (std::string(pathbuf).find(substr) != std::string::npos)
            {
                ++count;
            }
        }
    }
#endif

    return count;
}
#endif /* HAS_FD_INSPECTION */

TEST(BPMetadataFDClose, ReadRandomAccessClosesFDs)
{
    const std::string fname("BPMetadataFDCloseTest.bp");
    const size_t Nx = 10;
    const size_t NSteps = 3;

    adios2::ADIOS adios;

    /* Write a small file */
    {
        adios2::IO io = adios.DeclareIO("WriteIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }
        auto var = io.DefineVariable<double>("data", {Nx}, {0}, {Nx});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        std::vector<double> buf(Nx);
        for (size_t step = 0; step < NSteps; ++step)
        {
            for (size_t i = 0; i < Nx; ++i)
            {
                buf[i] = static_cast<double>(step * Nx + i);
            }
            writer.BeginStep();
            writer.Put(var, buf.data());
            writer.EndStep();
        }
        writer.Close();
    }

    /* Open in ReadRandomAccess and verify metadata FDs are closed */
    {
        adios2::IO io = adios.DeclareIO("ReadIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
        ASSERT_EQ(reader.Steps(), NSteps);

        /* Verify we can still read data (FD closing must not break reads) */
        auto var = io.InquireVariable<double>("data");
        ASSERT_TRUE(var);
        std::vector<double> buf(Nx);
        var.SetStepSelection({0, 1});
        reader.Get(var, buf.data(), adios2::Mode::Sync);
        EXPECT_EQ(buf[0], 0.0);
        EXPECT_EQ(buf[Nx - 1], static_cast<double>(Nx - 1));

#if HAS_FD_INSPECTION
        /* After Open in ReadRandomAccess on a completed file, the three
           metadata files (md.idx, md.0, mmd.0) should be closed. */
        int mdIdxFDs = CountOpenFDs("/md.idx");
        int md0FDs = CountOpenFDs("/md.0");
        int mmd0FDs = CountOpenFDs("/mmd.0");

        EXPECT_EQ(mdIdxFDs, 0) << "md.idx should be closed after ReadRandomAccess Open";
        EXPECT_EQ(md0FDs, 0) << "md.0 should be closed after ReadRandomAccess Open";
        EXPECT_EQ(mmd0FDs, 0) << "mmd.0 should be closed after ReadRandomAccess Open";
#endif

        reader.Close();
    }

    CleanupTestFiles(fname);
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    if (argc > 2)
    {
        engineParameters = std::string(argv[2]);
    }
    result = RUN_ALL_TESTS();
    return result;
}
