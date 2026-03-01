#ifndef __XRDHTTPSSIHANDLER_HH__
#define __XRDHTTPSSIHANDLER_HH__
/******************************************************************************/
/*                                                                            */
/*                   X r d H t t p S s i H a n d l e r . h h                  */
/*                                                                            */
/* HTTP External Handler that bridges HTTP/HTTPS requests to SSI services.   */
/* This allows SSI-based services (like ADIOS) to be accessed via HTTPS.     */
/*                                                                            */
/* Usage: Configure in xrootd.cf with:                                        */
/*   http.exthandler ssi libadios2_xrootd_http.so                             */
/*                                                                            */
/******************************************************************************/

#include "XrdHttp/XrdHttpExtHandler.hh"
#include "XrdSsi/XrdSsiProvider.hh"
#include "XrdSsi/XrdSsiRequest.hh"
#include "XrdSsi/XrdSsiResource.hh"
#include "XrdSsi/XrdSsiService.hh"
#include "XrdSys/XrdSysError.hh"

#include <condition_variable>
#include <mutex>
#include <string>

/******************************************************************************/
/*                   X r d H t t p S s i R e q u e s t                        */
/******************************************************************************/

// Concrete implementation of XrdSsiRequest for HTTP-originated requests
class XrdHttpSsiRequest : public XrdSsiRequest
{
public:
    XrdHttpSsiRequest(const char *reqData, int reqLen);
    ~XrdHttpSsiRequest() override;

    // Required: Return the request data buffer
    char *GetRequest(int &reqLen) override;

    // Required: Handle the response from the SSI service
    bool ProcessResponse(const XrdSsiErrInfo &eInfo, const XrdSsiRespInfo &rInfo) override;

    // Optional: Handle streaming response data
    void ProcessResponseData(const XrdSsiErrInfo &eInfo, char *buff, int blen, bool last) override;

    // Wait for response to be ready
    bool WaitForResponse(int timeoutSec = 30);

    // Get the response data after ProcessResponse is called
    bool GetResponseInfo(const char *&data, int &len, std::string &errMsg, int &errNum);

    // Check if response is an error
    bool IsError() const { return m_isError; }

private:
    char *m_reqData; // Request data buffer
    int m_reqLen;    // Request data length

    const char *m_respData; // Response data (points to rInfo data)
    int m_respLen;          // Response data length
    std::string m_errMsg;   // Error message if any
    int m_errNum;           // Error number if any
    bool m_isError;         // True if response is an error
    bool m_responseReady;   // True when response has been processed

    std::mutex m_mutex;
    std::condition_variable m_cond;
};

/******************************************************************************/
/*                   X r d H t t p S s i H a n d l e r                        */
/******************************************************************************/

class XrdHttpSsiHandler : public XrdHttpExtHandler
{
public:
    //--------------------------------------------------------------------------
    // XrdHttpExtHandler interface
    //--------------------------------------------------------------------------

    // Check if this handler should process the given request
    bool MatchesPath(const char *verb, const char *path) override;

    // Process an HTTP request
    int ProcessReq(XrdHttpExtReq &req) override;

    // Initialize the handler
    int Init(const char *cfgfile) override;

    //--------------------------------------------------------------------------
    // Constructor/Destructor
    //--------------------------------------------------------------------------

    XrdHttpSsiHandler(XrdSysError *log, const char *config, const char *parms, XrdOucEnv *myEnv);
    ~XrdHttpSsiHandler() override;

private:
    //--------------------------------------------------------------------------
    // Helper methods
    //--------------------------------------------------------------------------

    // Obtain the SSI service via dlsym (Option 1 - no XRootD modifications)
    bool ObtainSSIService();

    // Handle admin endpoint requests
    int ProcessAdminReq(XrdHttpExtReq &req);

    // Send an error response
    int SendError(XrdHttpExtReq &req, int code, const char *message);

    // Send a success response with data
    int SendResponse(XrdHttpExtReq &req, const char *data, int len,
                     const char *contentType = "application/octet-stream");

    //--------------------------------------------------------------------------
    // Member variables
    //--------------------------------------------------------------------------

    XrdSysError m_log;             // Error logging
    XrdSsiService *m_ssiService;   // SSI service pointer (obtained via dlsym)
    XrdSsiProvider *m_ssiProvider; // SSI provider pointer
    std::string m_pathPrefix;      // URL path prefix to match (default: "/ssi")
    std::string m_ssiLibPath;      // Path to SSI library (for dlopen)
    bool m_initialized;            // Whether initialization succeeded
};

#endif // __XRDHTTPSSIHANDLER_HH__
