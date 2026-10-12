/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include <CLI/CLI.hpp>

#include "adios2/helper/adiosLog.h"

size_t Settings::ndigits(size_t n) const
{
    /* Determine how many characters are needed to print n */
    static char digitstr[32];
    return static_cast<size_t>(snprintf(digitstr, 32, "%zu", n));
}

void Settings::displayHelp()
{
    std::cout << "Usage: adios_iotest -a appid -c config {-s | -w} {-d d1[,d2,..,dN] "
                 "| -D r1[,r2,..,rN]}"
                 "[-x "
                 "file]\n"
              << "  -a appID:  unique number for each application in the workflow\n"
              << "  -c config: data specification config file\n"
              << "  -d ...     define process decomposition:\n"
              << "      d1:        number of processes in 1st (slowest) dimension\n"
              << "      dN:        number of processes in Nth dimension\n"
              << "                 d1*d2*..*dN must equal the number of processes\n"
              << "   -D ...    define process decomposition ratio:\n"
              << "      r1:        ratio of process decomposition in the 1st "
                 "(slowest) dimension\n"
              << "      rN:        ratio of process decomposition in the Nth "
                 "dimension\n"
              << "                 r1xr2x..xrN must scale up to process count"
                 "count without remainder\n"
              << "  -s OR -w:  strong or weak scaling. \n"
              << "             Dimensions in config are treated accordingly\n"
              << "  -x file    ADIOS configuration XML file\n"
#ifdef ADIOS2_HAVE_HDF5_PARALLEL
              << "  --hdf5     Use native Parallel HDF5 instead of ADIOS for I/O\n"
#endif
              << "  -v         increase verbosity\n"
              << "  -h         display this help\n"
              << "  -F         turn on fixed I/O pattern explicitly\n"
              << "  -T         turn on multi-threaded MPI (needed by SST/MPI)\n"
              << "  -p         specify the path of the output explicitly\n"
              << "  -t         print and dump the timing measured by the I/O "
                 "timer\n\n";
}

size_t Settings::stringToNumber(const std::string &varName, const char *arg) const
{
    char *end;
    size_t retval = static_cast<size_t>(std::strtoull(arg, &end, 10));
    if (end[0] || errno == ERANGE)
    {
        adios2::helper::Throw<std::invalid_argument>(
            "Utils::adios_iotest", "settings", "stringToNumber",
            "Invalid value given for " + varName + ": " + std::string(arg));
    }
    return retval;
}

#ifdef _WIN32
#define strdup _strdup
#endif

int Settings::parseCSDecomp(const char *arg)
{
    char *argCopy;
    char *ratio;

    argCopy = strdup(arg);
    ratio = strtok(argCopy, ",");
    while (ratio)
    {
        processDecomp[nDecomp++] = stringToNumber("decomposition ratio", ratio);
        ratio = strtok(NULL, ",");
    }

    free(argCopy);

    return (0);
}

int Settings::rescaleDecomp()
{
    size_t ratioProd = 1;
    size_t scaleFactor;

    for (size_t i = 0; i < nDecomp; i++)
    {
        ratioProd *= processDecomp[i];
    }

    for (scaleFactor = 1; ratioProd * pow(scaleFactor, nDecomp) <= nProc; scaleFactor++)
    {
        if (ratioProd * pow(scaleFactor, nDecomp) == nProc)
        {
            for (size_t i = 0; i < nDecomp; i++)
            {
                processDecomp[i] *= scaleFactor;
            }
            return (0);
        }
    }

    adios2::helper::Throw<std::invalid_argument>(
        "Utils::adios_iotest", "settings", "rescaleDecomp",
        "decomposition ratios must scale up to process count");
    return 0;
}

int Settings::processArgs(int argc, char *argv[])
{
    bool help = false;
    bool strong = false;
    bool weak = false;
    bool ratio = false;
    std::vector<std::string> decomp;
    std::vector<std::string> decompRatio;

    CLI::App app;
    app.set_help_flag();
    app.add_flag("-h,--help", help);
    app.add_flag("-v,--verbose", verbose);
    auto *appIdOpt = app.add_option("-a,--appid", appId);
    app.add_option("-c,--config", configFileName);
    app.add_option("-d,--decomp", decomp)->expected(-1);
    app.add_option("-D,--decomp-ratio", decompRatio)->expected(-1);
    app.add_option("-x,--xml", adiosConfigFileName);
    app.add_option("-p,--path", outputPath);
    app.add_flag("-s,--strong-scaling", strong);
    app.add_flag("-w,--weak-scaling", weak);
    app.add_flag("-t,--timer", ioTimer);
    app.add_flag("-F,--fixed", fixedPattern);
    app.add_flag("-T,--multithreaded-mpi", multithreadedMPI);
#ifdef ADIOS2_HAVE_HDF5_PARALLEL
    bool hdf5 = false;
    app.add_flag("-H,--hdf5", hdf5);
#endif

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError &e)
    {
        adios2::helper::Throw<std::invalid_argument>("Utils::adios_iotest", "settings",
                                                     "processArgs", e.what());
    }

    if (help)
    {
        if (!myRank)
        {
            displayHelp();
        }
        return 1;
    }
#ifdef ADIOS2_HAVE_HDF5_PARALLEL
    if (hdf5)
    {
        iolib = IOLib::HDF5;
    }
#endif
    if (strong && weak)
    {
        adios2::helper::Throw<std::invalid_argument>("Utils::adios_iotest", "settings",
                                                     "processArgs",
                                                     "Cannot have -w and -s used at the same time");
    }
    if (!decomp.empty() && !decompRatio.empty())
    {
        adios2::helper::Throw<std::invalid_argument>("Utils::adios_iotest", "settings",
                                                     "processArgs",
                                                     "Cannot have -D and -d used at the same time");
    }
    ratio = !decompRatio.empty();
    isRatioDecomp = ratio;
    for (const std::string &arg : ratio ? decompRatio : decomp)
    {
        if (arg.find(',') != std::string::npos)
        {
            parseCSDecomp(arg.c_str());
        }
        else
        {
            processDecomp[nDecomp] = stringToNumber(
                "decomposition in dimension " + std::to_string(nDecomp + 1), arg.c_str());
            ++nDecomp;
        }
    }
    const bool appIdDefined = appIdOpt->count() > 0;
    const bool scalingDefined = strong || weak;
    isStrongScaling = !weak;

    /* Check if we have a everything defined */
    if (!appIdDefined)
    {
        adios2::helper::Throw<std::invalid_argument>(
            "Utils::adios_iotest", "settings", "processArgs",
            "Missing argument for application ID, which must be unique for "
            "each application (see -a option)");
    }
    if (configFileName.empty())
    {
        adios2::helper::Throw<std::invalid_argument>(
            "Utils::adios_iotest", "settings", "processArgs",
            "Missing argument for config file (see -c option)");
    }
    if (!scalingDefined)
    {
        adios2::helper::Throw<std::invalid_argument>(
            "Utils::adios_iotest", "settings", "processArgs",
            "Missing argument for scaling, which must be set to Strong or Weak "
            "(see -s, -w options)");
    }

    return 0;
}

int Settings::processArguments(int argc, char *argv[])
{
    int retval = 0;
    try
    {
        retval = processArgs(argc, argv);
    }
    catch (std::exception &e) // command-line argument errors
    {
        std::cout << "ERROR : " << e.what() << std::endl;
        displayHelp();
        retval = 1;
    }
    return retval;
}

int Settings::initDecomp(MPI_Comm worldComm)
{
    int retval = 0;
    try
    {
        int wrank;
        MPI_Comm_rank(worldComm, &wrank);
        MPI_Comm_split(worldComm, static_cast<int>(appId), wrank, &appComm);

        int rank, nproc;
        MPI_Comm_rank(appComm, &rank);
        MPI_Comm_size(appComm, &nproc);
        myRank = static_cast<size_t>(rank);
        nProc = static_cast<size_t>(nproc);

        if (isRatioDecomp)
        {
            rescaleDecomp();
        }
    }
    catch (std::exception &e) // command-line argument errors
    {
        std::cout << "ERROR : " << e.what() << std::endl;
        displayHelp();
        retval = 1;
    }
    return retval;
}

int Settings::extraArgumentChecks()
{
    if (!nDecomp && nProc > 1)
    {
        std::cout << "ERROR : Missing decomposition for parallel program (see "
                     "-d option)"
                  << std::endl;
        return 1;
    }

    size_t N = 1;
    for (size_t i = 0; i < nDecomp; ++i)
    {
        N *= processDecomp[i];
    }

    if (N != nProc)
    {
        std::cout << "ERROR : Product of decomposition values = " << N
                  << " must equal the number of processes = " << nProc << std::endl;
        return 1;
    }
    return 0;
}
