/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestHelpers.h : Common helper functions for ADIOS2 engine tests
 */

#ifndef TEST_ADIOS2_ENGINE_TESTHELPERS_H_
#define TEST_ADIOS2_ENGINE_TESTHELPERS_H_

#include <cstdlib>
#include <string>

// Helper function to cleanup test files/directories
inline void CleanupTestFiles(const std::string &path)
{
#ifdef _WIN32
    // Windows: use rmdir for directories, del for files
    // Try rmdir first (for .bp directories), fall back to del (for files)
    std::string cmd = "rmdir /s /q \"" + path + "\" 2>nul || del /q \"" + path + "\" 2>nul";
#else
    // Unix/Linux/macOS: use rm -rf
    std::string cmd = "rm -rf " + path;
#endif
    std::system(cmd.c_str());
}

#endif /* TEST_ADIOS2_ENGINE_TESTHELPERS_H_ */
