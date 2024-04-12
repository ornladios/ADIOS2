/******************************************************************************/
/*                                                                            */
/*                    X r d S s i S v S e r v i c e . c c                     */
/*                                                                            */
/* (c) 2017 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC02-76-SFO0515 with the Department of Energy              */
/*                                                                            */
/* This file is part of the XRootD software suite.                            */
/*                                                                            */
/* XRootD is free software: you can redistribute it and/or modify it under    */
/* the terms of the GNU Lesser General Public License as published by the     */
/* Free Software Foundation, either version 3 of the License, or (at your     */
/* option) any later version.                                                 */
/*                                                                            */
/* XRootD is distributed in the hope that it will be useful, but WITHOUT      */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or      */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public       */
/* License for more details.                                                  */
/*                                                                            */
/* You should have received a copy of the GNU Lesser General Public License   */
/* along with XRootD in a file called COPYING.LESSER (LGPL license) and file  */
/* COPYING (GPL license).  If not, see <http://www.gnu.org/licenses/>.        */
/*                                                                            */
/* The copyright holder's institutional names and contributor's names may not */
/* be used to endorse or promote products derived from this software without  */
/* specific prior written permission of the institution or contributor.       */
/******************************************************************************/

#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "XrdOuc/XrdOucTokenizer.hh"
#include "XrdSys/XrdSysPthread.hh"
#include "XrdSys/XrdSysTimer.hh"

#include "XrdSsi/XrdSsiErrInfo.hh"
#include "XrdSsi/XrdSsiRequest.hh"
#include "XrdSsi/XrdSsiResource.hh"

#include "XrdSsiSvService.hh"
#include "XrdSsiSvStreamActive.hh"
#include "XrdSsiSvStreamPassive.hh"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosFunctions.h"

using namespace std;

/******************************************************************************/
/*                               D e f i n e s                                */
/******************************************************************************/

#define TRACE(x) cerr << sName << ": " << x << endl

/******************************************************************************/
/*                      T h r e a d   I n t e r f a c e                       */
/******************************************************************************/

namespace
{
void *SvWecho(void *svP)
{
    XrdSsiSvService *sessP = (XrdSsiSvService *)svP;
    sessP->doWecho();
    return 0;
}

void *SvAdiosGet(void *svP)
{
    XrdSsiSvService *sessP = (XrdSsiSvService *)svP;
    sessP->doAdiosGet();
    return 0;
}
}

/******************************************************************************/
/*                        S t a t i c   M e m b e r s                         */
/******************************************************************************/

char XrdSsiSvService::alertMsg[256];
int XrdSsiSvService::alertNum = 0;

/******************************************************************************/
/*                         L o c a l   C l a s s e s                          */
/******************************************************************************/

namespace
{
class AlertMsg : public XrdSsiRespInfoMsg
{
public:
    void RecycleMsg(bool sent = true)
    {
        free(msgBuf);
        delete this;
    }

    AlertMsg(char *aMsg) : XrdSsiRespInfoMsg(strdup(aMsg), strlen(aMsg)) {}
    ~AlertMsg() {}
};
}

/******************************************************************************/
/*                              F i n i s h e d                               */
/******************************************************************************/

void XrdSsiSvService::Finished(XrdSsiRequest &rqstR, const XrdSsiRespInfo &rInfo, bool cancel)
{
    // We are called when the request has completed either because the response
    // was finally sent or the client cancelled the request. A request is
    // cancelled because the client requested it, unprovisioned the session, or
    // we lost the connection to the client. This provides us the opportunity to
    // reclaim any resources tied up with the request object.
    //
    const char *what = (cancel ? "cancelled" : "completed");

    TRACE("Request " << hex << &rqstR << dec << " has been " << what << " resp " << rInfo.State());

    // Now free up resources depending on how we responded
    //
    switch (rInfo.rType)
    {
    case XrdSsiRespInfo::isData:
        // We use a fixed buffer. Otherwise, we would release it.
        break;
    case XrdSsiRespInfo::isError:
        // While error messages are copied, the object releases them.
        break;
    case XrdSsiRespInfo::isFile:
        // We responded with a file so we should close it
        TRACE("Closing fd=" << rInfo.fdnum);
        close(rInfo.fdnum);
        break;
    case XrdSsiRespInfo::isStream:
        // If we responded with a stream then delete it now
        TRACE("Deleting stream");
        delete rInfo.strmP;
        break;
    case XrdSsiRespInfo::isNone:
        // If a response has not been posted then we must have
        // cancelled before a response could have been posted.
        return;
        break;
    default:
        TRACE("Unknown response type, cannot complete!");
        break;
    }

    // The agent is actually called for finished since it bound to the request.
    // Hence we can delete this object to avoid a memory leak.
    //
    // UnBindRequest();
    delete this;
}

/******************************************************************************/
/*                               P r e p a r e                                */
/******************************************************************************/

bool XrdSsiSvService::Prepare(XrdSsiErrInfo &eInfo, const XrdSsiResource &rDesc)
{
    // Note that on the server, arguments resource.hAvoid and argument timeout are
    // meaningless and always set to zero. So, we ignore these. The static here
    // is to test some features that server-side provision has. The resource name
    // simply becomes our session name.
    //
    static int busyCount = 0;
    const char *rDest, *rName = rDesc.rName.c_str();

    // Do some tracing
    //
    TRACE("User='" << rDesc.rUser.c_str() << "'"
                   << " rName= " << rName << " cgi='" << rDesc.rInfo.c_str() << "'");

    // We test the "busy" return. A session of "/.../busy" triggers this and
    // eventualy we will fail the provision. We ignore MT issues for this.
    //
    if (strstr(rName, "/busy"))
    {
        busyCount++;
        if (busyCount & 0x3)
            eInfo.Set("Testing busy return...", EBUSY, 3);
        else
            eInfo.Set("Resource not available.", EINTR);
        return false;
    }

    // Here we test how the client recovers from an ENOENT. In the presence of a
    // redirector, the client should seek an alternate endpoint. This is triggered
    // if the name of the file has 'enoent'.
    //
    if (strstr(rName, "enoent"))
    {
        eInfo.Set("Resource not found.", ENOENT);
        return false;
    }

    // Here we test redirection. This is triggered by a resource name that contains
    // ">hostname:port"; the client is redirected to hostname:port
    //
    if ((rDest = index(rName, '>')))
    {
        const char *Colon = rindex(rDest + 1, ':');
        char hdest[256];
        int port = 0, hlen = 0;
        if (Colon)
        {
            port = atoi(Colon + 1);
            hlen = Colon - rDest - 1;
        }
        if (port <= 0 || !hlen || hlen >= (int)sizeof(hdest))
            eInfo.Set("Invalid redirect destination.", EINVAL);
        else
        {
            strncpy(hdest, rDest + 1, hlen);
            hdest[hlen] = 0;
            eInfo.Set(hdest, EAGAIN, port);
        }
        return false;
    }

    // We have cleared all the hurdles, so return true.
    //
    return true;
}

/******************************************************************************/
/*                        P r o c e s s R e q u e s t                         */
/******************************************************************************/

// This method gets called when a request comes in from a client for this
// particular session. There may be many active sessions at the same time.

void XrdSsiSvService::ProcessRequest(XrdSsiRequest &reqRef, XrdSsiResource &resRef)
{
    XrdSsiSvService *agent;

    // Since we want to support multiple requests in a single service, the simplest
    // way of doing this is to create a shadow service object that will own the
    // and actually process the request. So the service object created at startup
    // time simply dispatches these shadow services. The framework is unaware of
    // this and the shadow session is, in effect, our agent. There are more
    // efficient ways of doing this (e.g. a dedicated task object) but this is OK.
    // The agent will delete itself when the request is actually finished.
    //
    agent = new XrdSsiSvService(resRef.rName.c_str());
    agent->ProcessRequest4Me(&reqRef);
}
/******************************************************************************/
/*         help function to split strings                                     */
/******************************************************************************/
std::vector<std::string> split (const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss (s);
    std::string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}
/******************************************************************************/
/*                     P r o c e s s R e q u e s t 4 M e                      */
/******************************************************************************/

void XrdSsiSvService::ProcessRequest4Me(XrdSsiRequest *rqstP)
{
    XrdSsiResponder::Status respStat;
    XrdOucTokenizer reqInfo(0);
    XrdSsiStream *sP;
    char *quest, *reqData, *reqArgs = 0;
    int reqSize, fd;

    // We must take ownership of the request. This is crucial as the request needs
    // to know its session and the responder needs to know the request target.
    // Since the session and responder are one in the same we don't specify the
    // responder as the third argument.
    //
    BindRequest(*rqstP);

    // OK, we are ready to go, get the request and display it
    //
    reqData = rqstP->GetRequest(reqSize);
    TRACE(hex << rqstP << dec << ' ' << reqSize << " byte request: " << reqData);

    // Attach the request buffer to our tokenizer (here requests are all ASCII).
    //
    reqInfo.Attach(reqData);

    // Here we get the first token of the request (the command)
    //
    if ((reqData = reqInfo.GetLine()))
        reqData = reqInfo.GetToken(&reqArgs);
    else
    {
        RespondErr("Request not specified.", EINVAL);
        return;
    }

    // Process as needed. We do minimal error checking.
    //
    if (!strcmp(reqData, "alert"))
    {
        bool myMsg = false;
        reqData = reqInfo.GetToken(&reqArgs);
        if (!reqData || !(*reqData))
        {
            alertNum = 0;
            reqArgs = 0;
        }
        else
            alertNum = atoi(reqData);
        if (!reqArgs || !(*reqArgs))
            myMsg = true;
        else
            strncpy(alertMsg, reqArgs, sizeof(alertMsg));
        if (alertNum <= 0)
            alertNum = 1;
        for (int i = 0; i < alertNum; i++)
        {
            if (myMsg)
                sprintf(alertMsg, "Alert msg %d", i + 1);
            AlertMsg *theMsg = new AlertMsg(alertMsg);
            Alert(*theMsg);
        }
        Respond("");
        return;
    }

    if (!strcmp(reqData, "delay"))
    {
        int dsec = atoi(reqArgs);
        if (dsec > 0)
            XrdSysTimer::Snooze(dsec);
        Respond("Delay completed.");
        return;
    }

    if (!strcmp(reqData, "echo"))
    {
        if ((quest = index(reqArgs, '?')))
        {
            *quest = 0;
            quest++;
        }
        Respond(reqArgs, quest);
        return;
    }

    if (!strcmp(reqData, "file"))
    {
        struct stat Stat;
        reqArgs = reqInfo.GetToken();
        if (!reqArgs || !(*reqArgs))
            RespondErr("File not specified.", EINVAL);
        else if ((fd = open(reqArgs, O_RDONLY)) < 0 || fstat(fd, &Stat))
            RespondErr(0, errno);
        else if ((respStat = SetResponse(Stat.st_size, fd)))
            ResponseFailed(respStat);
        return;
    }

    if (!strcmp(reqData, "relbuff"))
    {
        ReleaseRequestBuffer(); // Inherited from XrdSsiResponder!
        Respond("RelBuff() test completed.");
        return;
    }

    if (!strcmp(reqData, "stream")) // {active|passive} <sndsz> <file>
    {
        if (!reqArgs)
        {
            RespondErr("Stream args not specified.", EINVAL);
            return;
        }
        reqData = reqInfo.GetToken();
        if (reqData && !strcmp(reqArgs, "active"))
            streamActv = true;
        else if (reqData && !strcmp(reqArgs, "passive"))
            streamActv = false;
        else
        {
            RespondErr("Stream type is invalid.", EINVAL);
            return;
        }
        reqData = reqInfo.GetToken();
        streamRdSz = (reqData ? atoi(reqData) : 0);
        if (streamRdSz <= 0)
        {
            RespondErr("Stream sndsz is invalid.", EINVAL);
            return;
        }
        reqData = reqInfo.GetToken();
        if (!reqData || *reqData != '/')
        {
            RespondErr("Stream file is invalid.", EINVAL);
            return;
        }
        if ((fd = open(reqData, O_RDONLY)) < 0)
        {
            RespondErr("Unable to open file", errno);
            return;
        }
        if (streamActv)
            sP = new XrdSsiSvStreamActive(fd, streamRdSz);
        else
            sP = new XrdSsiSvStreamPassive(fd, streamRdSz);
        if ((respStat = SetResponse(sP)))
            ResponseFailed(respStat);
        return;
    }

    if (!strcmp(reqData, "wecho"))
    {
        pthread_t tid;
        reqData = reqInfo.GetToken(&reqArgs);
        respDly = atoi(reqData);
        if (!reqArgs || !(*reqArgs))
        {
            RespondErr("Echo string not specified.", EINVAL);
            return;
        }
        if ((quest = index(reqArgs, '?')))
        {
            Copy2Buff(respMeta, sizeof(respMeta), quest + 1, strlen(quest + 1) + 1);
            *quest = 0;
        }
        Copy2Buff(respBuff, sizeof(respBuff), reqArgs, strlen(reqArgs) + 1);
        XrdSysThread::Run(&tid, SvWecho, (void *)this, 0, "wecho");
        return;
    }
    if (!strcmp(reqData, "get" ))
    {
        pthread_t tid;
        reqData = reqInfo.GetToken(&reqArgs);
        respDly = atoi(reqData);
        if (!reqArgs || !(*reqArgs))
        {
            RespondErr("Arguments are expected", EINVAL);
            return;
        }
//        if ((quest = index(reqArgs, '?')))
//        {
//            Copy2Buff(respMeta, sizeof(respMeta), quest + 1, strlen(quest + 1) + 1);
//            *quest = 0;
//        }
        /* parameters  name, step, blockID, count0, count1, count2, ...  start0, start1, start2 */
        std::vector<std::string> requestParams = split (reqArgs, '&');

        m_io = adios.DeclareIO("xtoord");
        m_engine = m_io.Open(reqData, adios2::Mode::ReadRandomAccess);
        std::string VarName = requestParams[0];
        adios2::DataType TypeOfVar = m_io.InquireVariableType(VarName);
        try
        {
            if (TypeOfVar == adios2::DataType::None)
            {
            }
#define GET(T)                                                                                     \
    else if (TypeOfVar == adios2::helper::GetDataType<T>())                                        \
    {                                                                                              \
        adios2::Variable<T> var = m_io.InquireVariable<T>(VarName);                                \
        std::vector<T> resBuffer;                                                                  \
        size_t step = std::stoi(requestParams[1]);                                                 \
        var.SetStepSelection({step, step + 1});                                                    \
        size_t paramLength = (requestParams.size() - 3) / 2;                                       \
        adios2::Dims s(paramLength);                                                               \
        adios2::Dims c(paramLength);                                                               \
        for (auto i = 0; i < paramLength; i++)                                                     \
        {                                                                                          \
            c[i] = std::stoi(requestParams[3 + i]);                                                \
            s[i] = std::stoi(requestParams[3 + paramLength + i]);                                  \
        }                                                                                          \
        adios2::Box<adios2::Dims> varSel(s, c);                                                    \
        var.SetSelection(varSel);                                                                  \
        m_engine.Get(var, resBuffer, adios2::Mode::Sync);                                          \
        size_t responseSize = resBuffer.size();                                                    \
        responseBuffer = new char[responseSize * sizeof(float)];                                   \
        responseBufferSize = responseSize * sizeof(float);                                         \
        memcpy(responseBuffer, resBuffer.data(), responseSize * sizeof(float));                    \
        XrdSysThread::Run(&tid, SvAdiosGet, (void *)this, 0, "get");                               \
    }
            ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(GET)
#undef GET
        }
    catch (const std::exception &exc)
    {
        RespondErr("Returning exception for Get ", EINVAL);
    }
        // detached thread. Memory should not be deallocated yet,
        // olnly of a thread is finished
        return;
    }
    // Ok we don't know what this is
    //
    RespondErr("Invalid request.", EINVAL);
}

/******************************************************************************/
/*                        H e l p e r   M e t h o d s                         */
/******************************************************************************/
/******************************************************************************/
/*                             C o p y 2 B u f f                              */
/******************************************************************************/

int XrdSsiSvService::Copy2Buff(char *dest, int dsz, const char *src, int ssz)
{
    if (ssz <= dsz)
    {
        memcpy(dest, src, ssz);
        return ssz;
    }

    memcpy(dest, src, dsz - 1);
    dest[dsz - 1] = 0;
    return dsz;
}

/******************************************************************************/
/*                               R e s p o n d                                */
/******************************************************************************/

void XrdSsiSvService::Respond(const char *rData, const char *mData)
{
    XrdSsiResponder::Status rc;
    int rLen;

    // Handle meta data first. We copy the metadata into a buffer that must live
    // even after we return because the data may be sent later.
    //
    if (mData && *mData)
    {
        rLen = strlen(mData) + 1;
        if (mData != respMeta)
            rLen = Copy2Buff(respMeta, sizeof(respMeta), mData, rLen);
        if ((rc = SetMetadata(respMeta, rLen)))
            ResponseFailed(rc);
    }

    // Check if all we are doing is sending metadata
    //
    if (!(*rData))
    {
        if ((rc = SetNilResponse()))
            ResponseFailed(rc);
        return;
    }

    // We copy the response into a buffer that must live even after we return to
    // the caller because the data in that buffer will be sent back to the client.
    //
    rLen = strlen(rData) + 1;
    if (rData != respBuff)
        rLen = Copy2Buff(respBuff, sizeof(respBuff), rData, rLen);

    // We use the inherited method XrdSsiResponder::SetResponse to post the response
    // Note we always send the null byte to make it easy on the client :-)
    //
    if ((rc = SetResponse(respBuff, rLen)))
        ResponseFailed(rc);
}
void XrdSsiSvService::AdiosRespond(const char *rData, const char *mData)
{
    XrdSsiResponder::Status rc;
    int rLen;

    // Handle meta data first. We copy the metadata into a buffer that must live
    // even after we return because the data may be sent later.
    //
    if (mData && *mData)
    {
        rLen = strlen(mData) + 1;
        if (mData != respMeta)
            rLen = Copy2Buff(respMeta, sizeof(respMeta), mData, rLen);
        if ((rc = SetMetadata(respMeta, rLen)))
            ResponseFailed(rc);
    }

    // Check if all we are doing is sending metadata
    //
    //    if (!(*rData))
    //    {if ((rc = SetNilResponse())) ResponseFailed(rc);
    //        return;
    //    }

    // We copy the response into a buffer that must live even after we return to
    // the caller because the data in that buffer will be sent back to the client.
    //
    // rLen = strlen(rData)+1;
    rLen = responseBufferSize;
    if (rData != respBuff)
        rLen = Copy2Buff(respBuff, sizeof(respBuff), rData, rLen);

    // We use the inherited method XrdSsiResponder::SetResponse to post the response
    // Note we always send the null byte to make it easy on the client :-)
    //
    if ((rc = SetResponse(respBuff, rLen)))
        ResponseFailed(rc);
}

/******************************************************************************/
/*                            R e s p o n d E r r                             */
/******************************************************************************/

void XrdSsiSvService::RespondErr(const char *eText, int eNum)
{
    XrdSsiResponder::Status respStat;

    if (!eText)
        eText = strerror(eNum);
    respStat = SetErrResponse(eText, eNum);
    if (respStat != XrdSsiResponder::wasPosted)
        ResponseFailed(respStat);
}

/******************************************************************************/
/*                        R e s p o n s e F a i l e d                         */
/******************************************************************************/

void XrdSsiSvService::ResponseFailed(XrdSsiResponder::Status rc)
{

    // Determine why we could not set a response. This should never happen for our
    // simple example as everything is serialized.
    //
    switch (rc)
    {
    case notPosted:
        TRACE("Response failed; duplicate response");
        break;
    case notActive:
        TRACE("Response failed; request inactive");
        delete this;
        break;
    default:
        TRACE("Response failed; unknown error");
        break;
    }
}

/******************************************************************************/
/* Private:                    S e n d A l e r t                              */
/******************************************************************************/

void XrdSsiSvService::SendAlert(char *aMsg, int aNum)
{
    XrdSsiRespInfoMsg *theMsg;
    char mBuff[256];

    // Send alert
    //
    for (int i = 0; i < aNum; i++)
    {
        snprintf(mBuff, sizeof(mBuff), "%d: %s", i + 1, aMsg);
        theMsg = new AlertMsg(mBuff);
        Alert(*theMsg);
    }
}
