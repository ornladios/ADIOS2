/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adios2_libinfo.h
 *
 *  Created on: June 22, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "adios2_libinfo.h"
#include "adios2/common/ADIOSConfig.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

const int adios2_version_major = ADIOS2_VERSION_MAJOR;
const int adios2_version_minor = ADIOS2_VERSION_MINOR;
const int adios2_version_patch = ADIOS2_VERSION_PATCH;
const char *const adios2_version_str = ADIOS2_VERSION_STR;

static const char *aae[] = {"BP3",
                            "BP4",
#ifdef ADIOS2_HAVE_BP5
                            "BP5",
#endif
#ifdef ADIOS2_HAVE_HDF5
                            "HDF5",
#endif
#ifdef ADIOS2_HAVE_SST
                            "SST",
#endif
#ifdef ADIOS2_HAVE_MPI
                            "SSC",
#endif
#ifdef ADIOS2_HAVE_DataMan
                            "DataMan",
#endif
#ifdef ADIOS2_HAVE_DataSpaces
                            "DataSpaces",
#endif
                            "Inline",
#ifdef ADIOS2_HAVE_DAOS
                            "DAOS",
#endif
#ifdef ADIOS2_HAVE_MHS
                            "MHS",
#endif
#ifdef ADIOS2_HAVE_CATALYST
                            "ParaViewADIOSInSituEngine",
#endif
                            "Null",
                            "Skeleton",
                            nullptr};

void adios2_available_engines(int *nentries, char const ***list)
{
    int ne = 0;
    while (aae[ne] != nullptr)
        ++ne;
    *nentries = ne;
    *list = aae;
    return;
}

static const char *aao[] = {
#ifdef ADIOS2_HAVE_BZIP2
    "BZip2",
#endif
#ifdef ADIOS2_HAVE_BLOSC2
    "Blosc",
#endif
#ifdef ADIOS2_HAVE_MGARD
    "MGARD",
    "MGARDPlus",
#endif
#ifdef ADIOS2_HAVE_SZ
    "SZ",
#endif
#ifdef ADIOS2_HAVE_ZFP
    "ZFP",
#endif
#ifdef ADIOS2_HAVE_PNG
    "PNG",
#endif
#ifdef ADIOS2_HAVE_SIRIUS
    "Sirius",
#endif
#ifdef ADIOS2_HAVE_LIBPRESSIO
    "libpressio",
#ifdef ADIOS2_HAVE_SODIUM
    "Sodium plugin",
#endif
#endif
    "None",
    nullptr};

void adios2_available_operators(int *nentries, char const ***list)
{
    int no = 0;
    while (aao[no] != nullptr)
        ++no;
    *nentries = no;
    *list = aao;
    return;
}

static const char *aaf[] = {
#ifdef ADIOS2_HAVE_MPI
    "MPI",
#endif
#ifdef ADIOS2_HAVE_FORTRAN
    "Fortran",
#endif
#ifdef ADIOS2_HAVE_PYTHON
    "Python",
#endif
#ifdef ADIOS2_HAVE_IME
    "IME",
#endif
#ifdef ADIOS2_HAVE_UCX
    "UCX",
#endif
#ifdef ADIOS2_HAVE_AWSSDK
    "AWSSDK",
#endif

#ifdef ADIOS2_HAVE_KOKKOS
    "Kokkos (Host"
#ifdef ADIOS2_HAVE_KOKKOS_CUDA
    ", CUDA"
#endif
#ifdef ADIOS2_HAVE_KOKKOS_HIP
    ", HIP"
#endif
#ifdef ADIOS2_HAVE_KOKKOS_SYCL
    ", SYCL"
#endif
    ")"
#endif // ADIOS2_HAVE_KOKKOS

#ifdef ADIOS2_HAVE_CUDA
    "CUDA",
#endif
    nullptr};

void adios2_available_features(int *nentries, char const ***list)
{
    int nf = 0;
    while (aaf[nf] != nullptr)
        ++nf;
    *nentries = nf;
    *list = aaf;
    return;
}

void adios2_free_list(int nentries, char const ***list) {}

#ifdef __cplusplus
} // end extern C
#endif
