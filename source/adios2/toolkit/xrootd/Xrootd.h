/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 * ganyushin@gmail.com
 */

#ifndef ADIOS2_XROOTD_H
#define ADIOS2_XROOTD_H

#include "adios2/common/ADIOSConfig.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"
#ifdef ADIOS2_HAVE_XRootD
#include "XrdSsi/XrdSsiProvider.hh"
#include "XrdSsi/XrdSsiService.hh"
#endif
namespace adios2
{
class Xrootd
{

public:
    profiling::IOChrono m_Profiler;
#ifdef ADIOS2_HAVE_XRootD
    XrdSsiProvider *XrdSsiProviderClient;
    XrdSsiErrInfo          eInfo;
#endif
    int                    reqLen;
    std::string contact;

    Xrootd();
    ~Xrootd();

    void Open(const std::string hostname, const int32_t port, const std::string filename,
              const Mode mode, bool RowMajorOrdering);

    typedef int GetHandle;

    GetHandle Get(char *VarName, size_t Step, size_t BlockID, Dims &Count, Dims &Start, void *dest);

    GetHandle Read(size_t Start, size_t Size, void *Dest);

    int64_t m_ID;
    size_t m_Size;
};

} // end namespace adios2

#endif // ADIOS2_XROOTD_H
