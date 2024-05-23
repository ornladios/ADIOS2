#ifndef __XRDSSISVSERVICE_HH__
#define __XRDSSISVSERVICE_HH__
/******************************************************************************/
/*                                                                            */
/*                    X r d S s i S v S e r v i c e . h h                     */
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

#include <unistd.h>

#include "XrdSsi/XrdSsiResponder.hh"
#include "XrdSsi/XrdSsiService.hh"

#include "adios2.h"
// ADIOS initialization
//  Here we define out service implementation. We need to inherit the Service
//  class but equally important we need to also inherit the Responder class in
//  order to post a response to the request object.
#include "AdiosFilePool.h"

class XrdSsiSvService : public XrdSsiService, public XrdSsiResponder
{
public:
    //-----------------------------------------------------------------------------
    //! Attach to a previously detached server-side request.
    //-----------------------------------------------------------------------------

    virtual void Attach(XrdSsiRequest &reqRef, std::string handle) {}

    //-----------------------------------------------------------------------------
    //! Do delayed echo (internal)
    //-----------------------------------------------------------------------------

    void doWecho()
    {
        Respond(m_responseBuffer, m_respMeta);
    }

    void doAdiosGet() { AdiosRespond(m_responseBuffer, m_respMeta); }

    //-----------------------------------------------------------------------------
    //! Prepare for request arrival.
    //-----------------------------------------------------------------------------

    virtual bool Prepare(XrdSsiErrInfo &eInfo, const XrdSsiResource &resource);

    //-----------------------------------------------------------------------------
    //! Process a new request (on server-side a new thread is used for this call).
    //-----------------------------------------------------------------------------

    virtual void ProcessRequest(XrdSsiRequest &reqRef, XrdSsiResource &resRef);

    //-----------------------------------------------------------------------------
    //! Constructor
    //-----------------------------------------------------------------------------

    XrdSsiSvService(const char *sname = 0)
    {
        sName = strdup(sname ? sname : "");
        *m_respMeta = 0;
    }

    XrdSsiSvService(const char *sname, ADIOSFilePool *parentPoolPointer = NULL)
    {
        sName = strdup(sname ? sname : "");
        *m_respMeta = 0;
        m_FilePoolPtr = parentPoolPointer;
    }

protected:
    //-----------------------------------------------------------------------------
    //! Notify a service that a request either completed or was canceled. This
    //! method allows the service to release any resources given to the request
    //! object (e.g. data response buffer or a stream) before it looses control.
    //-----------------------------------------------------------------------------

    virtual void Finished(XrdSsiRequest &rqstR, const XrdSsiRespInfo &rInfo, bool cancel = false);

    //-----------------------------------------------------------------------------
    //! Destructor is protected. You cannot use delete on a service, use the
    //! Stop() method to effectively delete the service object.
    //-----------------------------------------------------------------------------
  ~XrdSsiSvService();

private:
    int Copy2Buff(char *dest, int dsz, const char *src, int ssz);
    void ProcessRequest4Me(XrdSsiRequest *reqP);
    void Respond(const char *rData, const char *mData = 0);
    void AdiosRespond(const char *rData, const char *mData = 0);
    void RespondErr(const char *eText, int eNum);
    void ResponseFailed(XrdSsiResponder::Status rc);
    void SendAlert(char *aMsg, int aNum);

    static char alertMsg[256];
    static int alertNum;
    char *sName;
    char *m_responseBuffer = NULL;
    int m_responseBufferSize = 0;
    char m_respMeta[512];
    char m_respData[1024];
    int streamRdSz;
    bool streamActv;
    adios2::ADIOSFilePool m_ParentFilePool; // unused except in parent object
    adios2::ADIOSFilePool *m_FilePoolPtr;   // pointer to parent object pool
};
#endif
