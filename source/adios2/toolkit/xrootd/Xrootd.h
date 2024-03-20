/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 * ganyushin@gmail.com
 */

#ifndef ADIOS2_XROOTD_H
#define ADIOS2_XROOTD_H
#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/ADIOS.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"
#ifdef ADIOS2_HAVE_XROOTD
#include "XrdOuc/XrdOucStream.hh"
#include "XrdSsi/XrdSsiProvider.hh"
#include "XrdSsi/XrdSsiRequest.hh"
#include "XrdSsi/XrdSsiResource.hh"
#include "XrdSsi/XrdSsiService.hh"
#include "XrdSys/XrdSysPthread.hh"
#endif
class XrdSsiClUI
{
public:
    enum Action
    {
        CancelRequest = 0,
        RunRequest,
        StopService
    };

    Action GetRequest(int &reqLen);

    bool ParseCL(int argc, char **argv);

    char *cmdName;
    const char *contact;
    char *usrName;
    char *cgiInfo;
    char *avoids;
    char *runName;
    char *reqID;
    int detTTL;
    int timeOut;
    int readSZ;
    uint32_t resOpts;
    XrdSsiResource::Affinity affVal;
    bool isAsync;
    bool doEcho;
    bool doOnce;
    bool doNop;
    XrdSsiService *ssiService;
    char *reqBuff;

    XrdSsiClUI();
    ~XrdSsiClUI() {}

private:
    void DoSet();
    void DoSource();
    bool GetNum(const char *opT, char *opV, int &opD);
    long long GetSize(const char *tName, char *var, long long maxV);
    char *GetToken(const char *tName);
    void Help();
    void SetAff(const char *aName);
    bool Usage();

    XrdSysMutex uiMutex;
    XrdOucStream cLine;
    char prompt[80];
    static const int minBsz = 1024;            // 1 KB
    static const int maxBsz = 8 * 1024 * 1024; // 8 MB
    const char *affName;
    int reqBsz;
    int reqBpad;
    int srcFD;
};
namespace adios2
{
class Xrootd
{

public:
    profiling::IOChrono m_Profiler;
#ifdef ADIOS2_HAVE_XROOTD
    XrdSsiProvider *XrdSsiProviderClient;
    XrdSsiErrInfo eInfo;
#endif
    int reqLen;
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
