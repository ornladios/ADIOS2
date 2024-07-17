/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 * ganyushin@gmail.com
 */

#ifndef ADIOS2_XROOTDREMOTE_H
#define ADIOS2_XROOTDREMOTE_H
#include "Remote.h"
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
    bool isAsync;
    bool doEcho;
    bool doOnce;
    bool doNop;
#ifdef ADIOS2_HAVE_XROOTD
    XrdSsiResource::Affinity affVal;
    XrdSsiService *ssiService;
#endif
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

#ifdef ADIOS2_HAVE_XROOTD
    XrdSysMutex uiMutex;
    XrdOucStream cLine;
#endif
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
class XrootdRemote : public Remote
{

public:
    profiling::IOChrono m_Profiler;
#ifdef ADIOS2_HAVE_XROOTD

    XrdSsiErrInfo eInfo;
#endif
    int reqLen;
    std::string m_Filename;
    Mode m_Mode;
    bool m_RowMajorOrdering;

    XrootdRemote(const adios2::HostOptions &hostOptions);
    ~XrootdRemote();

    void Open(const std::string hostname, const int32_t port, const std::string filename,
              const Mode mode, bool RowMajorOrdering);

    GetHandle Get(char *VarName, size_t Step, size_t BlockID, Dims &Count, Dims &Start, void *dest);

    GetHandle Read(size_t Start, size_t Size, void *Dest);
    bool WaitForGet(GetHandle handle);
};

} // end namespace adios2

#endif // ADIOS2_XROOTDREMOTE_H
