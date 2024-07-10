/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 * ganyushin@gmail.com
 */
#include <future>

#include "XrootdRemote.h"
#include "adios2/core/ADIOS.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosString.h"
#include "adios2/helper/adiosSystem.h"
#ifdef ADIOS2_HAVE_XROOTD
#include "XrdSsi/XrdSsiProvider.hh"
#include "XrdSsi/XrdSsiRequest.hh"
#include "XrdSsi/XrdSsiService.hh"

/******************************************************************************/
/*                               D e f i n e s                                */
/******************************************************************************/

#define FATAL(x)                                                                                   \
    {                                                                                              \
        cerr << "ssicl: " << x << '\n' << flush;                                                   \
        exit(3);                                                                                   \
    }

/******************************************************************************/
/*                       N a m e s p a c e   I t e m s                        */
/******************************************************************************/

namespace XrdSsiCl
{
const char *cmdName = "ssicl: ";

FILE *outErr = fdopen(2, "w");

FILE *outFile = outErr;
}

/******************************************************************************/
/*                      F i l e   L o c a l   I t e m s                       */
/******************************************************************************/
namespace
{
XrdSysMutex gMutex;

XrdSsiClUI clUI;
}

class myRequest : public XrdSsiRequest
{
public:
    void Alert(XrdSsiRespInfoMsg &aMsg);

    void Cancel();

    char *GetRequest(int &dlen)
    {
        dlen = reqBlen;
        return reqBuff;
    }

    bool ProcessResponse(const XrdSsiErrInfo &eInfo, const XrdSsiRespInfo &rInfo);

    void ProcessResponseData(const XrdSsiErrInfo &eInfo, char *buff, int dlen, bool last);

    void RelRequestBuffer() {} //{if (reqBuff) free(reqBuff); reqBuff = 0;}

    void SetResource(XrdSsiResource &resR) { rSpec = resR; }

    myRequest(XrdSsiClUI &uiP, char *rname, char *reqid, char *buff, int blen)
    : XrdSsiRequest(reqid), rSpec(std::string("")), rName(strdup(rname)), theID(reqid),
      reqBuff(buff), reqBlen(blen), totbytes(0), cancelled(false), retry(true)
    {
        readSZ = clUI.readSZ;
        rspBuff = (char *)malloc(clUI.readSZ);
        SetTimeOut(clUI.timeOut);
        SetDetachTTL(clUI.detTTL);
        gMutex.Lock();
        currentRequest = this;

        gMutex.UnLock();
    }

    ~myRequest()
    {
        if (reqBuff)
            free(reqBuff);
        if (rspBuff)
            free(rspBuff);
        if (rName)
            free(rName);
        if (theID)
            free(theID);
        gMutex.Lock();
        if (currentRequest == this)
            currentRequest = 0;
        gMutex.UnLock();
    }

    static myRequest *currentRequest;
    char *responseBuffer;
    int responseBufferLen;
    char *dest;
    std::promise<void> *promise;

private:
    XrdSsiResource rSpec;
    char *rName;
    char *theID;
    char *reqBuff;
    int reqBlen;
    int totbytes;
    int readSZ;
    char *rspBuff;
    bool cancelled;
    bool retry;
};
myRequest *myRequest::currentRequest = 0; // Pointer to current request

/******************************************************************************/
/*                      m y R e q u e s t : : A l e r t                       */
/******************************************************************************/

void myRequest::Alert(XrdSsiRespInfoMsg &aMsg)
{
    char *theMsg;
    int theMsz;

    // Get the message
    //
    theMsg = aMsg.GetMsg(theMsz);

    // Print what we received
    //
    //    fprintf(XrdSsiCl::outFile, "%s@%s: Rcvd %d bytes alert: '%s'\n", rName,
    //    GetEndPoint().c_str(),
    //            theMsz, theMsg);

    // Recycle the message
    //
    aMsg.RecycleMsg();
}

/******************************************************************************/
/*                     m y R e q u e s t : : C a n c e l                      */
/******************************************************************************/

void myRequest::Cancel()
{
    bool finStatus;

    // First mark that we were cancelled to avoid calling finished twice.
    //
    cancelled = true;

    // Now tell the request we are done because we want to cancel it
    //
    finStatus = Finished(true);

    // Now if tyhe cancel was not accepted then someone else got to call
    // finish first. Be aware this is only for testing as we are not doing proper
    // locking to avoid refering to a deleted object.
    //
    if (finStatus)
        fprintf(XrdSsiCl::outErr, "Current %s request cancelled.\n", rName);
    else
        fprintf(XrdSsiCl::outErr,
                "Request %s not cancelled; "
                "no longer valid.\n",
                rName);
}

/******************************************************************************/
/*            m y R e q u e s t : : P r o c e s s R e s p o n s e             */
/******************************************************************************/

bool myRequest::ProcessResponse(const XrdSsiErrInfo &eInfo, const XrdSsiRespInfo &rInfo)
{

    // Check if processing completed correctly, If not, issue message and delete
    // the request object (i.e. ourselves).
    //
    if (!eInfo.isOK())
    {
        fprintf(XrdSsiCl::outFile, "Request %s@%s failed; %s\n", rName, GetEndPoint().c_str(),
                rInfo.eMsg);
        Finished();
        if (retry)
        {
            retry = false;
            fprintf(XrdSsiCl::outFile, "Retrying request....\n");
            clUI.ssiService->ProcessRequest(*this, rSpec);
        }
        else
            delete this;
        return true;
    }

    // We do some debugging here
    //
    // const char *theMD; int theML;
    // theMD = GetMetadata(theML);
    if (rInfo.mdlen)
    {
        //        fprintf(XrdSsiCl::outFile,
        //                "%s@%s: Rcvd %s response "
        //                "with %d metabytes '%s'\n",
        //                rName, GetEndPoint().c_str(), rInfo.State(), rInfo.mdlen, rInfo.mdata);
    }
    else
    {
        // fprintf(XrdSsiCl::outFile, "%s@%s: Rcvd %s response\n", rName, GetEndPoint().c_str(),
        //           rInfo.State());
    }

    // While a response can have one of several forms a good response can only be
    // isStream or isData. GetResponseData() can handle either one.
    //
    GetResponseData(rspBuff, readSZ);
    return true;
}

/******************************************************************************/

void myRequest::ProcessResponseData(const XrdSsiErrInfo &eInfo, char *buff, int dlen, bool last)
{

    // Check if the request for response data ended with an error. If so, we tell
    // the service we are finished with this request and delete simply ourselves.
    //
    if (dlen < 0 || !eInfo.isOK())
    {
        fprintf(XrdSsiCl::outFile,
                "Unable to get data from service "
                "%s@%s; %s\n",
                rName, GetEndPoint().c_str(), eInfo.Get().c_str());
        Finished();
        promise->set_value();
        delete this;
        return;
    }

    // Now we check if we need to ask for more data. If last is false, we do!
    //
    if (!last && !clUI.doOnce)
    {
        GetResponseData(rspBuff, readSZ);
        return;
    }

    // fill in destination buffer
    //
    totbytes += dlen;
    memcpy(dest, buff, dlen);

    // We are done with our request. We avoid calling Finished if we got here
    // because we were cancelled.
    //
    promise->set_value();
    Finished();
    delete this;
}

/******************************************************************************/
/*                              G e t R e q I D                               */
/******************************************************************************/

char *GetReqID() { return (clUI.reqID ? strdup(clUI.reqID) : 0); }
/******************************************************************************/
/*                           C o n s t r u c t o r                            */
/******************************************************************************/

XrdSsiClUI::XrdSsiClUI()
{
    contact = 0;
    usrName = 0;
    cgiInfo = 0;
    avoids = 0;
    runName = 0;
    reqID = 0;
    detTTL = 0;
    timeOut = 0;
    readSZ = 1024 * 1024;
    resOpts = 0;
    isAsync = false;
    doEcho = true;
    doOnce = false;
    doNop = false;
    affName = "default";
    affVal = XrdSsiResource::Default;
    ssiService = 0;
    reqBuff = 0;
    reqBsz = 4096;
    reqBpad = 0;
    srcFD = 0;
    strcpy(prompt, "> ");

    cLine.Attach(STDIN_FILENO);
}
extern XrdSsiProvider *XrdSsiProviderClient;
#endif
namespace adios2
{
XrootdRemote::XrootdRemote(const adios2::HostOptions &hostOptions) : Remote(hostOptions) {}
XrootdRemote::~XrootdRemote() {}
void XrootdRemote::Open(const std::string hostname, const int32_t port, const std::string filename,
                        const Mode mode, bool RowMajorOrdering)
{
#ifdef ADIOS2_HAVE_XROOTD
    m_Filename = filename;
    m_Mode = mode;
    m_RowMajorOrdering = RowMajorOrdering;

    const std::string contact = hostname + ":" + std::to_string(port);
    clUI.cmdName = strdup("adios:");
    clUI.contact = strdup(contact.c_str());
    if (!(clUI.ssiService = XrdSsiProviderClient->GetService(eInfo, contact.c_str())))
    {
        fprintf(XrdSsiCl::outErr, "Unable to get service object for %s; %s\n", clUI.contact,
                eInfo.Get().c_str());
    }
#endif
    return;
}

bool XrootdRemote::WaitForGet(GetHandle handle)
{
    std::promise<void> *p = (std::promise<void> *)handle;
    p->get_future().wait();
    delete p;
    return true;
}

Remote::GetHandle XrootdRemote::Get(char *VarName, size_t Step, size_t BlockID, Dims &Count,
                                    Dims &Start, void *dest)
{
#ifdef ADIOS2_HAVE_XROOTD
    char rName[512] = "/home/eisen/xroot/data";
    XrdSsiResource rSpec((std::string)rName);
    myRequest *reqP;
    std::string reqData = "get Filename=" + std::string(m_Filename) + std::string("&RMOrder=") +
                          std::to_string(m_RowMajorOrdering) + std::string("&Varname=") +
                          std::string(VarName);
    reqData += "&Step=" + std::to_string(Step);
    reqData += "&Block=" + std::to_string(BlockID);
    reqData += "&Dims=" + std::to_string(Count.size());

    for (auto &c : Count)
    {
        reqData += "&Count=" + std::to_string(c);
    }

    for (auto &s : Start)
    {
        reqData += "&Start=" + std::to_string(s);
    }

    // The first step is to define the resource we will be using. So, just
    // initialize a resource object. It need only to exist during ProcessRequest()
    //
    if (clUI.usrName)
        rSpec.rUser = clUI.usrName;
    if (clUI.cgiInfo)
        rSpec.rInfo = clUI.cgiInfo;
    if (clUI.avoids)
        rSpec.hAvoid = clUI.avoids;
    rSpec.affinity = clUI.affVal;
    rSpec.rOpts = clUI.resOpts;
    reqLen = strlen(reqData.c_str()) + 1;

    // Normally, we would maintain a pool of request objects to avoid new/delete
    // calls. For our simple client we have no context so we simply always get a
    // new object.
    //
    char *reqDataStr = strdup(reqData.c_str());
    reqP = new myRequest(clUI, rName, GetReqID(), reqDataStr, reqLen);
    reqP->SetResource(rSpec);
    reqP->dest = (char *)dest;
    reqP->promise = new std::promise<void>();
    // We simply hand off the request to the service to deal with it. When a
    // response is ready or an error occured our callback is invoked.
    //
    clUI.ssiService->ProcessRequest(*reqP, rSpec);
    // thread synchronization
    return (GetHandle)(intptr_t)(reqP->promise);
#else
    return (GetHandle)(intptr_t)0;
#endif
}

XrootdRemote::GetHandle XrootdRemote::Read(size_t Start, size_t Size, void *Dest)
{
    return static_cast<GetHandle>(0);
};

}
