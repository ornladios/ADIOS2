/******************************************************************************/
/*                                                                            */
/*                X r d H t t p S s i H a n d l e r . c c                     */
/*                                                                            */
/* HTTP External Handler that bridges HTTP/HTTPS requests to SSI services.   */
/*                                                                            */
/******************************************************************************/

#include "XrdHttpSsiHandler.hh"

#include "XrdOuc/XrdOucEnv.hh"
#include "XrdSec/XrdSecEntity.hh"
#include "XrdVersion.hh"

#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <sstream>

XrdVERSIONINFO(XrdHttpGetExtHandler, HttpSsi);

/******************************************************************************/
/*                               D e f i n e s                                */
/******************************************************************************/

#define TRACE(x) m_log.Emsg("HttpSsi", x)

/******************************************************************************/
/*             X r d H t t p S s i R e q u e s t   M e t h o d s              */
/******************************************************************************/

XrdHttpSsiRequest::XrdHttpSsiRequest(const char *reqData, int reqLen)
: m_reqLen(reqLen), m_respData(nullptr), m_respLen(0), m_errNum(0), m_isError(false),
  m_responseReady(false)
{
    // Copy request data - SSI framework expects us to own this buffer
    m_reqData = (char *)malloc(reqLen + 1);
    if (m_reqData)
    {
        memcpy(m_reqData, reqData, reqLen);
        m_reqData[reqLen] = '\0';
    }
}

XrdHttpSsiRequest::~XrdHttpSsiRequest()
{
    if (m_reqData)
    {
        free(m_reqData);
        m_reqData = nullptr;
    }
}

char *XrdHttpSsiRequest::GetRequest(int &reqLen)
{
    reqLen = m_reqLen;
    return m_reqData;
}

bool XrdHttpSsiRequest::ProcessResponse(const XrdSsiErrInfo &eInfo, const XrdSsiRespInfo &rInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check for errors first
    if (eInfo.hasError())
    {
        m_isError = true;
        m_errMsg = eInfo.Get();
        m_errNum = eInfo.GetArg();
        m_responseReady = true;
        m_cond.notify_all();
        return true;
    }

    // Process based on response type
    switch (rInfo.rType)
    {
    case XrdSsiRespInfo::isData:
        // Direct data response
        m_respData = rInfo.buff;
        m_respLen = rInfo.blen;
        m_isError = false;
        break;

    case XrdSsiRespInfo::isError:
        // Error response
        m_isError = true;
        m_errMsg = rInfo.eMsg ? rInfo.eMsg : "Unknown error";
        m_errNum = rInfo.eNum;
        break;

    case XrdSsiRespInfo::isStream:
        // Stream response - for now, we don't support this via HTTP
        // A full implementation would use chunked transfer encoding
        m_isError = true;
        m_errMsg = "Stream responses not yet supported via HTTP";
        m_errNum = ENOTSUP;
        break;

    case XrdSsiRespInfo::isFile:
        // File response - for now, we don't support this via HTTP
        m_isError = true;
        m_errMsg = "File responses not yet supported via HTTP";
        m_errNum = ENOTSUP;
        break;

    case XrdSsiRespInfo::isNone:
        // No response data
        m_respData = nullptr;
        m_respLen = 0;
        m_isError = false;
        break;

    default:
        m_isError = true;
        m_errMsg = "Unknown response type";
        m_errNum = EINVAL;
        break;
    }

    m_responseReady = true;
    m_cond.notify_all();
    return true;
}

void XrdHttpSsiRequest::ProcessResponseData(const XrdSsiErrInfo &eInfo, char *buff, int blen,
                                            bool last)
{
    // This is called for streaming responses
    // For now, we accumulate data but a full implementation would
    // stream it back via HTTP chunked transfer encoding
    std::lock_guard<std::mutex> lock(m_mutex);

    if (eInfo.hasError())
    {
        m_isError = true;
        m_errMsg = eInfo.Get();
        m_errNum = eInfo.GetArg();
    }

    if (last)
    {
        m_responseReady = true;
        m_cond.notify_all();
    }
}

bool XrdHttpSsiRequest::WaitForResponse(int timeoutSec)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_responseReady)
        return true;

    auto status =
        m_cond.wait_for(lock, std::chrono::seconds(timeoutSec), [this] { return m_responseReady; });
    return status;
}

bool XrdHttpSsiRequest::GetResponseInfo(const char *&data, int &len, std::string &errMsg,
                                        int &errNum)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isError)
    {
        errMsg = m_errMsg;
        errNum = m_errNum;
        data = nullptr;
        len = 0;
        return false;
    }
    data = m_respData;
    len = m_respLen;
    return true;
}

/******************************************************************************/
/*            X r d H t t p S s i H a n d l e r   M e t h o d s               */
/******************************************************************************/

XrdHttpSsiHandler::XrdHttpSsiHandler(XrdSysError *log, const char *config, const char *parms,
                                     XrdOucEnv *myEnv)
: m_log(log->logger(), "HttpSsi_"), m_ssiService(nullptr), m_ssiProvider(nullptr),
  m_pathPrefix("/ssi"), m_ssiLibPath(""), m_initialized(false)
{
    // Parse parameters if provided (e.g., "prefix=/adios ssilib=/path/to/lib.so")
    if (parms && *parms)
    {
        std::string p(parms);
        std::istringstream iss(p);
        std::string token;
        while (iss >> token)
        {
            if (token.find("prefix=") == 0)
            {
                m_pathPrefix = token.substr(7);
            }
            else if (token.find("ssilib=") == 0)
            {
                m_ssiLibPath = token.substr(7);
            }
        }
    }

    m_log.Emsg("Init", "HTTP-SSI handler created, path prefix:", m_pathPrefix.c_str());
    if (!m_ssiLibPath.empty())
    {
        m_log.Emsg("Init", "SSI library path:", m_ssiLibPath.c_str());
    }
}

XrdHttpSsiHandler::~XrdHttpSsiHandler()
{
    // Don't delete m_ssiService - it's owned by the SSI framework
}

int XrdHttpSsiHandler::Init(const char *cfgfile)
{
    // Try to obtain the SSI service
    if (!ObtainSSIService())
    {
        m_log.Emsg("Init", "Failed to obtain SSI service - handler will reject requests");
        return 0; // Return success anyway, will fail requests later
    }

    m_initialized = true;
    m_log.Emsg("Init", "HTTP-SSI handler initialized successfully");
    return 0;
}

bool XrdHttpSsiHandler::ObtainSSIService()
{
    void *handle = nullptr;

    // If we have a specific SSI library path, open it directly
    // Use RTLD_NOLOAD to get a handle to the already-loaded library
    if (!m_ssiLibPath.empty())
    {
        m_log.Emsg("ObtainSSI", "Opening SSI library:", m_ssiLibPath.c_str());
        handle = dlopen(m_ssiLibPath.c_str(), RTLD_NOW | RTLD_NOLOAD);
        if (!handle)
        {
            // Library not already loaded, try loading it
            m_log.Emsg("ObtainSSI", "Library not pre-loaded, trying to load it");
            handle = dlopen(m_ssiLibPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
        }
    }
    else
    {
        // Fallback: try to find in the main executable (may not work if not RTLD_GLOBAL)
        handle = dlopen(NULL, RTLD_NOW);
    }

    if (!handle)
    {
        m_log.Emsg("ObtainSSI", "dlopen failed:", dlerror());
        return false;
    }

    // Look for the XrdSsiProviderServer symbol
    XrdSsiProvider **providerPtr = (XrdSsiProvider **)dlsym(handle, "XrdSsiProviderServer");

    if (!providerPtr)
    {
        m_log.Emsg("ObtainSSI", "XrdSsiProviderServer symbol not found:", dlerror());
        dlclose(handle);
        return false;
    }

    m_ssiProvider = *providerPtr;
    if (!m_ssiProvider)
    {
        m_log.Emsg("ObtainSSI", "XrdSsiProviderServer is NULL");
        dlclose(handle);
        return false;
    }

    // Get the service object
    XrdSsiErrInfo eInfo;
    m_ssiService = m_ssiProvider->GetService(eInfo, "", 256);

    if (!m_ssiService)
    {
        std::string err = eInfo.Get();
        m_log.Emsg("ObtainSSI", "GetService failed:", err.empty() ? "unknown error" : err.c_str());
        dlclose(handle);
        return false;
    }

    m_log.Emsg("ObtainSSI", "Successfully obtained SSI service");
    // Note: We don't dlclose here as we need the library to stay loaded
    return true;
}

bool XrdHttpSsiHandler::MatchesPath(const char *verb, const char *path)
{
    // Match POST requests to our prefix path
    // Format: POST /ssi/<resource>
    if (!verb || !path)
        return false;

    // Accept POST for requests, GET for simple queries
    if (strcmp(verb, "POST") != 0 && strcmp(verb, "GET") != 0)
    {
        return false;
    }

    // Check if path starts with our prefix
    return (strncmp(path, m_pathPrefix.c_str(), m_pathPrefix.length()) == 0);
}

int XrdHttpSsiHandler::ProcessReq(XrdHttpExtReq &req)
{
    // Check if we're initialized
    if (!m_initialized || !m_ssiService)
    {
        return SendError(req, 503, "SSI service not available");
    }

    // Extract the resource name from the path (after prefix)
    std::string resource = req.resource;
    if (resource.length() > m_pathPrefix.length())
    {
        resource = resource.substr(m_pathPrefix.length());
    }
    else
    {
        resource = "/";
    }

    // For GET requests, the SSI command might be in query parameters
    // For POST requests, the SSI command is in the request body
    std::string ssiCommand;

    if (req.verb == "GET")
    {
        // Check for xrd-http-query header (contains query string)
        auto it = req.headers.find("xrd-http-query");
        if (it != req.headers.end())
        {
            ssiCommand = it->second;
        }
        else
        {
            return SendError(req, 400, "GET request requires query parameters");
        }
    }
    else
    {
        // POST - read the request body
        if (req.length <= 0)
        {
            return SendError(req, 400, "POST request requires body");
        }

        // Read the request body
        char *bodyData = nullptr;
        int bytesRead = req.BuffgetData(req.length, &bodyData, true);
        if (bytesRead <= 0 || !bodyData)
        {
            return SendError(req, 400, "Failed to read request body");
        }
        ssiCommand = std::string(bodyData, bytesRead);
    }

    // Create the SSI resource
    XrdSsiResource ssiResource(resource);

    // Create the SSI request with the command
    XrdHttpSsiRequest *ssiReq = new XrdHttpSsiRequest(ssiCommand.c_str(), ssiCommand.length());

    // Process the request
    m_ssiService->ProcessRequest(*ssiReq, ssiResource);

    // Wait for the response (with timeout)
    if (!ssiReq->WaitForResponse(300))
    {
        delete ssiReq;
        return SendError(req, 504, "SSI request timeout");
    }

    // Get the response
    const char *respData = nullptr;
    int respLen = 0;
    std::string errMsg;
    int errNum = 0;

    if (!ssiReq->GetResponseInfo(respData, respLen, errMsg, errNum))
    {
        // Error response
        delete ssiReq;
        std::stringstream ss;
        ss << "SSI error (" << errNum << "): " << errMsg;
        return SendError(req, 500, ss.str().c_str());
    }

    // Success - send the response
    int result = SendResponse(req, respData, respLen);

    // Clean up - call Finished to release resources
    ssiReq->Finished();
    delete ssiReq;

    return result;
}

int XrdHttpSsiHandler::SendError(XrdHttpExtReq &req, int code, const char *message)
{
    m_log.Emsg("SendError", message);
    return req.SendSimpleResp(code, nullptr, nullptr, const_cast<char *>(message), strlen(message));
}

int XrdHttpSsiHandler::SendResponse(XrdHttpExtReq &req, const char *data, int len,
                                    const char *contentType)
{
    std::string headers = "Content-Type: ";
    headers += contentType;

    return req.SendSimpleResp(200, nullptr, const_cast<char *>(headers.c_str()),
                              const_cast<char *>(data), len);
}

/******************************************************************************/
/*                  X r d H t t p G e t E x t H a n d l e r                   */
/******************************************************************************/

extern "C" {

XrdHttpExtHandler *XrdHttpGetExtHandler(XrdSysError *log, const char *config, const char *parms,
                                        XrdOucEnv *myEnv)
{
    log->Emsg("HttpSsiInit", "Loading HTTP-SSI bridge handler");

    try
    {
        XrdHttpSsiHandler *handler = new XrdHttpSsiHandler(log, config, parms, myEnv);
        if (handler->Init(config) != 0)
        {
            log->Emsg("HttpSsiInit", "Handler initialization failed");
            delete handler;
            return nullptr;
        }
        return handler;
    }
    catch (const std::exception &e)
    {
        log->Emsg("HttpSsiInit", "Exception during handler creation:", e.what());
        return nullptr;
    }
}
}
