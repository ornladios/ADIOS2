/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * settings.h
 *
 *  Created on: Oct 2018
 *      Author: Norbert Podhorszki
 */

#include "settings.h"

#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <string>

struct option options[] = {{"help", no_argument, NULL, 'h'},
                           {"verbose", no_argument, NULL, 'v'},
                           {"appid", required_argument, NULL, 'a'},
                           {"config", required_argument, NULL, 'c'},
                           {"decomp", required_argument, NULL, 'd'},
                           {"xml", required_argument, NULL, 'x'},
                           {"strong-scaling", no_argument, NULL, 's'},
                           {"weak-scaling", no_argument, NULL, 'w'},
#ifdef ADIOS2_HAVE_HDF5
                           {"hdf5", no_argument, NULL, 'H'},
#endif
                           {NULL, 0, NULL, 0}};

static const char *optstring = "-hvswHa:c:d:x:";

size_t Settings::ndigits(size_t n) const
{
    /* Determine how many characters are needed to print n */
    static char digitstr[32];
    return static_cast<size_t>(snprintf(digitstr, 32, "%zu", n));
}

void Settings::displayHelp()
{
    std::cout
        << "Usage: adios_iotest -a appid -c config {-s | -w} -d d1 [d2 .. dN] "
           "[-x "
           "file]\n"
        << "  -a appID:  unique number for each application in the workflow\n"
        << "  -c config: data specification config file\n"
        << "  -d ...     define process decomposition:\n"
        << "      d1:        number of processes in 1st (slowest) dimension\n"
        << "      dN:        number of processes in Nth dimension\n"
        << "                 d1*d2*..*dN must equal the number of processes\n"
        << "  -s OR -w:  strong or weak scaling. \n"
        << "             Dimensions in config are treated accordingly\n"
        << "  -x file    ADIOS configuration XML file\n"
#ifdef ADIOS2_HAVE_HDF5
        << "  --hdf5     Use native Parallel HDF5 instead of ADIOS for I/O"
#endif
        << "  -v         increase verbosity\n"
        << "  -h         display this help\n\n";
}

size_t Settings::stringToNumber(const std::string &varName,
                                const char *arg) const
{
    char *end;
    size_t retval = static_cast<size_t>(std::strtoull(arg, &end, 10));
    if (end[0] || errno == ERANGE)
    {
        throw std::invalid_argument("Invalid value given for " + varName +
                                    ": " + std::string(arg));
    }
    return retval;
}

int Settings::processArgs(int argc, char *argv[])
{
    bool appIdDefined = false;
    bool scalingDefined = false;
    int c;
    int last_c = '_';

    /* Process the arguments */
    while ((c = getopt_long(argc, argv, optstring, options, NULL)) != -1)
    {
        switch (c)
        {
        case 'a':
            appId = stringToNumber("appID", optarg);
            appIdDefined = true;
            break;
        case 'c':
            configFileName = optarg;
            break;
        case 'd':
            processDecomp[nDecomp] =
                stringToNumber("decomposition in dimension 1", optarg);
            ++nDecomp;
            break;
        case 'h':
            if (!myRank)
            {
                displayHelp();
            }
            return 1;
#ifdef ADIOS2_HAVE_HDF5
        case 'H':
            iolib = IOLib::HDF5;
            break;
#endif
        case 's':
            if (scalingDefined && !isStrongScaling)
            {
                throw std::invalid_argument(
                    "Cannot have -w and -s used at the same time ");
            }
            isStrongScaling = true;
            scalingDefined = true;
            break;
        case 'v':
            ++verbose;
            break;
        case 'w':
            if (scalingDefined && isStrongScaling)
            {
                throw std::invalid_argument(
                    "Cannot have -s and -w used at the same time ");
            }
            isStrongScaling = false;
            scalingDefined = true;
            break;
        case 'x':
            adiosConfigFileName = optarg;
            break;
        case 1:
            /* This means a field is unknown, or could be multiple arg or bad
             * arg*/

            if (last_c == 'd')
            { // --decomp extra arg (or not if not a number)
                processDecomp[nDecomp] = stringToNumber(
                    "decomposition in dimension " + std::to_string(nDecomp + 1),
                    optarg);
                ++nDecomp;
            }
            else
            {
                throw std::invalid_argument("Invalid argument " +
                                            std::string(optarg));
            }
            break;

        default:
            throw std::invalid_argument("Invalid argument option " +
                                        std::string(optarg));
            break;
        } /* end switch */
        if (c != 1)
        {
            last_c = c;
        }
    } /* end while */

    /* Check if we have extra unprocessed arguments */
    if (optind < argc)
    {
        std::string s;
        while (optind < argc)
        {
            s += std::string(argv[optind]) + " ";
            ++optind;
        }
        throw std::invalid_argument("There are unknown arguments: " + s);
    }

    /* Check if we have a everything defined */
    if (!appIdDefined)
    {
        throw std::invalid_argument("Missing argument for application ID, "
                                    "which must be unique for each application "
                                    "(see -a option)");
    }
    if (configFileName.empty())
    {
        throw std::invalid_argument(
            "Missing argument for config file (see -c option)");
    }
    if (!scalingDefined)
    {
        throw std::invalid_argument("Missing argument for scaling, "
                                    "which must be set to Strong or Weak "
                                    "(see -s, -w options)");
    }

    return 0;
}

int Settings::processArguments(int argc, char *argv[], MPI_Comm worldComm)
{
    int retval = 0;
    try
    {
        retval = processArgs(argc, argv);

        int wrank;
        MPI_Comm_rank(worldComm, &wrank);
        MPI_Comm_split(worldComm, appId, wrank, &appComm);

        int rank, nproc;
        MPI_Comm_rank(appComm, &rank);
        MPI_Comm_size(appComm, &nproc);
        myRank = static_cast<size_t>(rank);
        nProc = static_cast<size_t>(nproc);
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
                  << " must equal the number of processes = " << nProc
                  << std::endl;
        return 1;
    }
    return 0;
}
