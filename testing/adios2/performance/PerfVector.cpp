/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <chrono>
#include <iostream>
#include <stdexcept>

#include <adios2/helper/adiosType.h>

#ifndef _WIN32
#include "strings.h"
#else
#define strcasecmp _stricmp
#endif

size_t MB = 1024 * 1024ULL;
size_t N_MB = 1;

static void Usage()
{
    std::cout << "PerfVector N " << std::endl;
    std::cout << "  N is size of vector in MiB" << std::endl;
}

static bool ParseArgs(int argc, char **argv)
{
    if (argc > 1)
    {
        std::istringstream ss(argv[1]);
        if (!(ss >> N_MB))
            std::cerr << "Invalid number for size " << argv[1] << '\n';
    }
    else
    {
        Usage();
        return false;
    }
    return true;
}

using TP = std::chrono::high_resolution_clock::time_point;
#define NOW() std::chrono::high_resolution_clock::now();
#define DURATION(T1, T2) static_cast<double>((T2 - T1).count()) / 1000000000.0;

int main(int argc, char **argv)
{
    if (!ParseArgs(argc, argv))
    {
        return 1;
    }

    double timeInitialized, timeUninitialized, timeMalloc;
    size_t N = N_MB * MB;

    {
        TP startInitialized = NOW();
        std::vector<char> vecInitialized(N);
        // vecInitialized.resize(N);
        // char *ptr = vecInitialized.data();
        // for (size_t i = 0; i < N; ++i)
        //{
        //    vecInitialized[i] = i % 256;
        //}
        TP endInitialized = NOW();
        timeInitialized = DURATION(startInitialized, endInitialized);
    }

    {
        TP startUninitialized = NOW();
        // std::vector<char, default_init_allocator<char>> vecUninitialized(N);
        adios2::helper::adiosvec<char> vecUninitialized(N);
        // vecUninitialized.resize(N);
        // char *ptr = vecUninitialized.data();
        // for (size_t i = 0; i < N; ++i)
        //{
        //    vecUninitialized[i] = i % 256;
        //}
        TP endUninitialized = NOW();
        timeUninitialized = DURATION(startUninitialized, endUninitialized);
    }

    {
        TP startMalloc = NOW();
        char *vecMalloc = (char *)malloc(N);
        // char *ptr = vecMalloc;
        /*for (size_t i = 0; i < N; ++i)
        {
            *ptr = i % 256;
            ++ptr;
        }*/
        // for (size_t i = 0; i < N; ++i)
        //{
        //    vecMalloc[i] = i % 256;
        //}
        TP endMalloc = NOW();
        timeMalloc = DURATION(startMalloc, endMalloc);
        free(vecMalloc);
    }

    std::cout << "Size = " << N << " std::vector = " << timeInitialized
              << " helper::adiosvec = " << timeUninitialized
              << " Malloc = " << timeMalloc << std::endl;

    return 0;
}
