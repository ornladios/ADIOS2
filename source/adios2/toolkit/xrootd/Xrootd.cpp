/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 * ganyushin@gmail.com
 */
#include "Xrootd.h"
#include "adios2/core/ADIOS.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosString.h"
#include "adios2/helper/adiosSystem.h"
#ifdef ADIOS2_HAVE_XROOTD
#include "XrdSsi/XrdSsiProvider.hh"
#include "XrdSsi/XrdSsiService.hh"
#endif


namespace adios2
{
Xrootd::Xrootd(){
    contact = "localhost:1094";
}
Xrootd::~Xrootd(){}
void Xrootd::Open(const std::string hostname, const int32_t port, const std::string filename,
                  const Mode mode, bool RowMajorOrdering)
{
#ifdef ADIOS2_HAVE_XROOTD
    // TODO
#endif
    return;
}

Xrootd::GetHandle Xrootd::Get(char *VarName, size_t Step, size_t BlockID, Dims &Count, Dims &Start,
                              void *dest)
{
#ifdef ADIOS2_HAVE_XROOTD
    // TODO
#endif
    return 0;
}

}
