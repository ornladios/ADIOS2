/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestHelpers.h : Common helper functions for ADIOS2 engine tests
 */

#ifndef TEST_ADIOS2_ENGINE_TESTHELPERS_H_
#define TEST_ADIOS2_ENGINE_TESTHELPERS_H_

#include <algorithm>
#include <cstdlib>
#include <string>

// Helper function to cleanup test files/directories
// If engine is specified, only cleanup for file engines (not streaming engines)
// If engine is empty/null, always cleanup (default behavior for non-staging tests)
inline void CleanupTestFiles(const std::string &path, const std::string &engine = "")
{
    // If engine is specified, check if it's a file engine
    if (!engine.empty())
    {
        std::string engineLower = engine;
        std::transform(engineLower.begin(), engineLower.end(), engineLower.begin(), ::tolower);

        // File engines: bpfile, file, bp, bp3, bp4, bp5, hdfmixer, hdf5, skeleton, null
        // Streaming engines: filestream, bp4_stream, dataman, ssc, sst, effis, insitumpi, inline
        bool isFileEngine = (engineLower == "bpfile" || engineLower == "file" ||
                             engineLower == "bp" || engineLower == "bp3" ||
                             engineLower == "bp4" || engineLower == "bp5" ||
                             engineLower == "hdfmixer" || engineLower == "hdf5" ||
                             engineLower == "skeleton" || engineLower == "null");

        if (!isFileEngine)
        {
            return; // Don't cleanup for streaming engines
        }
    }

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
