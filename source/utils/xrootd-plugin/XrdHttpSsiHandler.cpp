/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

#include <cctype>
#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{
std::string UrlEncode(const std::string &s)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/')
        {
            out += c;
        }
        else
        {
            out += '%';
            out += hex[(c >> 4) & 0x0F];
            out += hex[c & 0x0F];
        }
    }
    return out;
}

// ---------------------------------------------------------------------
// Path-encoded URL decoder.
//
// Wire grammar matches the client encoder in
// source/adios2/toolkit/remote/XrootdHttpRemote.cpp:
//
//   /adios/<urlenc-filename>/<file-config>/<request>
//
//   file-config:   r<digit>?p<base64url-EP>?    (or `_` placeholder)
//   single get:    g~<base64url-var>~<paramstring>     (or `_` if no params)
//   batch get:     b~N~<v1>~<p1>~<v2>~<p2>~...~<vN>~<pN>
//
//   Variable names and EngineParams are base64url-encoded (RFC 4648 §5)
//   because they may contain '/' which HTTP intermediaries decode from
//   percent-encoding, breaking path-segment parsing.
//
//   paramstring:   letter-prefixed fields, no internal delimiter; fields
//                  self-delimit because the next field's letter ends the
//                  previous value.  Vector elements separated by `,`.
//
// Letter codes (single get / batch sub-request):
//   b Block            s StepStart        S StepCount
//   c Count            o Start            a AccuracyError
//   N AccuracyNorm     R AccuracyRelative
//
// File-config codes (in this order):
//   v WireVersion (digits)   r RMOrder (single digit)   u FileUUID (digits)
//   e ClientBigEndian (digits)   p EngineParams (rest of segment)
//
// The decoder builds a legacy "verb Filename=...&key=val&..." string for
// the inner SSI parser, which is unchanged.
// ---------------------------------------------------------------------

// Base64url decoding (RFC 4648 §5).  Inverse of the client's
// Base64urlEncode — uses '-' and '_' instead of '+' and '/',
// no padding required.
std::string Base64urlDecode(const std::string &in)
{
    // Lookup: ASCII value → 6-bit index (64 = invalid).
    // Covers 0x00–0xFF (256 entries).  Indices:
    //   A-Z = 0-25, a-z = 26-51, 0-9 = 52-61, '-' = 62, '_' = 63
    auto decodeCh = [](unsigned char c) -> unsigned char {
        if (c >= 'A' && c <= 'Z')
            return c - 'A';
        if (c >= 'a' && c <= 'z')
            return c - 'a' + 26;
        if (c >= '0' && c <= '9')
            return c - '0' + 52;
        if (c == '-')
            return 62;
        if (c == '_')
            return 63;
        return 64;
    };
    std::string out;
    out.reserve(in.size() * 3 / 4);
    unsigned int buf = 0;
    int bits = 0;
    for (unsigned char c : in)
    {
        if (c == '=')
            break;
        unsigned char v = decodeCh(c);
        if (v == 64)
            continue; // skip unknown chars
        buf = (buf << 6) | v;
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            out += static_cast<char>((buf >> bits) & 0xFF);
        }
    }
    return out;
}

std::vector<std::string> SplitOn(const std::string &s, char delim)
{
    std::vector<std::string> out;
    size_t start = 0;
    while (true)
    {
        size_t pos = s.find(delim, start);
        if (pos == std::string::npos)
        {
            out.push_back(s.substr(start));
            return out;
        }
        out.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
}

// Map a per-request paramstring letter to the legacy field name.  Returns
// nullptr if the character isn't a known field letter, which doubles as
// the "this character belongs in the value, not a new field" test.
const char *PerRequestKey(char letter)
{
    switch (letter)
    {
    case 'b':
        return "Block";
    case 's':
        return "StepStart";
    case 'S':
        return "StepCount";
    case 'c':
        return "Count";
    case 'o':
        return "Start";
    case 'a':
        return "AccuracyError";
    case 'N':
        return "AccuracyNorm";
    case 'R':
        return "AccuracyRelative";
    default:
        return nullptr;
    }
}

// Append `&Key=Value` pairs for each field in a per-request paramstring.
// `_` placeholder produces no output.  Returns false on malformed input.
bool DecodePerRequestParamString(const std::string &paramstring, std::ostringstream &out)
{
    if (paramstring == "_")
        return true;
    size_t i = 0;
    const size_t n = paramstring.size();
    while (i < n)
    {
        const char *key = PerRequestKey(paramstring[i]);
        if (!key)
            return false;
        ++i;
        const size_t valStart = i;
        while (i < n && PerRequestKey(paramstring[i]) == nullptr)
            ++i;
        if (i == valStart)
            return false;
        out << "&" << key << "=" << paramstring.substr(valStart, i - valStart);
    }
    return true;
}

// Decode the file-config segment: optional v, r, u, e in that order, then p.
// `_` placeholder is empty.
bool DecodeFileConfigSegment(const std::string &fileConfig, std::ostringstream &out)
{
    if (fileConfig == "_")
        return true;
    size_t i = 0;
    const size_t n = fileConfig.size();
    if (i < n && fileConfig[i] == 'v')
    {
        ++i;
        const size_t start = i;
        while (i < n && fileConfig[i] >= '0' && fileConfig[i] <= '9')
            ++i;
        if (i == start)
            return false; // 'v' with no digits
        out << "&WireVersion=" << fileConfig.substr(start, i - start);
    }
    if (i < n && fileConfig[i] == 'r')
    {
        ++i;
        if (i >= n)
            return false;
        char d = fileConfig[i++];
        if (d != '0' && d != '1')
            return false;
        out << "&RMOrder=" << d;
    }
    if (i < n && fileConfig[i] == 'u')
    {
        ++i;
        const size_t start = i;
        while (i < n && fileConfig[i] >= '0' && fileConfig[i] <= '9')
            ++i;
        if (i == start)
            return false; // 'u' with no digits
        out << "&FileUUID=" << fileConfig.substr(start, i - start);
    }
    if (i < n && fileConfig[i] == 'e')
    {
        ++i;
        const size_t start = i;
        while (i < n && fileConfig[i] >= '0' && fileConfig[i] <= '9')
            ++i;
        if (i == start)
            return false; // 'e' with no digits
        out << "&ClientBigEndian=" << fileConfig.substr(start, i - start);
    }
    if (i < n && fileConfig[i] == 'p')
    {
        ++i;
        out << "&EngineParams=" << UrlEncode(Base64urlDecode(fileConfig.substr(i)));
        return true;
    }
    return i == n;
}

// Top-level decoder.  Strips the route prefix, splits the path, and
// composes the legacy ssiCommand from the three segments.  On parse
// error returns false with an explanation in `errorOut`.
bool DecodePathEncodedRequest(const std::string &fullresource, const std::string &pathPrefix,
                              std::string &ssiCommandOut, std::string &resourceOut,
                              std::string &errorOut)
{
    // Strip query string defensively (path-encoded form puts everything
    // in the path; any `?…` is an artifact and we ignore it).
    std::string path = fullresource;
    size_t qpos = path.find('?');
    if (qpos != std::string::npos)
        path = path.substr(0, qpos);

    const std::string fullPrefix = pathPrefix + "/";
    if (path.compare(0, fullPrefix.size(), fullPrefix) != 0)
    {
        errorOut = "URL does not start with " + fullPrefix;
        return false;
    }
    path = path.substr(fullPrefix.size());

    // Three segments after the prefix: filename, file-config, request.
    // The filename's slashes are literal path separators (not encoded),
    // so it spans multiple segments.  The last two are file-config and
    // request; everything before them is the filename.
    std::vector<std::string> segments = SplitOn(path, '/');
    if (segments.size() < 3)
    {
        errorOut = "URL must have filename, file-config, and request segments";
        return false;
    }
    const std::string requestSeg = segments.back();
    segments.pop_back();
    const std::string fileConfigSeg = segments.back();
    segments.pop_back();
    std::string encodedFilename;
    for (size_t i = 0; i < segments.size(); ++i)
    {
        if (i > 0)
            encodedFilename += "/";
        encodedFilename += segments[i];
    }

    // Ensure leading '/' so the resource matches the legacy form
    // (XRootD SSI uses it for tar-archive resolution and caching).
    if (encodedFilename.empty() || encodedFilename[0] != '/')
        resourceOut = "/" + encodedFilename;
    else
        resourceOut = encodedFilename;

    if (requestSeg.size() < 2 || requestSeg[1] != '~')
    {
        errorOut = "Request segment missing op marker";
        return false;
    }
    const char op = requestSeg[0];
    std::string verb;
    if (op == 'g')
        verb = "get";
    else if (op == 'b')
        verb = "batchget";
    else
    {
        errorOut = std::string("Unknown op marker '") + op + "' in request segment";
        return false;
    }

    std::ostringstream cmd;
    cmd << verb << " Filename=" << resourceOut;

    if (!DecodeFileConfigSegment(fileConfigSeg, cmd))
    {
        errorOut = "Malformed file-config segment '" + fileConfigSeg + "'";
        return false;
    }

    if (op == 'g')
    {
        // g~<varname>~<paramstring>
        std::vector<std::string> parts = SplitOn(requestSeg.substr(2), '~');
        if (parts.size() != 2)
        {
            errorOut = "Single-get request must be 'g~<varname>~<params>'";
            return false;
        }
        cmd << "&Varname=" << UrlEncode(Base64urlDecode(parts[0]));
        if (!DecodePerRequestParamString(parts[1], cmd))
        {
            errorOut = "Malformed paramstring '" + parts[1] + "'";
            return false;
        }
    }
    else // op == 'b'
    {
        // b~N~<v1>~<p1>~<v2>~<p2>~...~<vN>~<pN>
        std::vector<std::string> parts = SplitOn(requestSeg.substr(2), '~');
        if (parts.empty())
        {
            errorOut = "Batch request missing sub-request count";
            return false;
        }
        size_t expected = 0;
        try
        {
            expected = std::stoul(parts[0]);
        }
        catch (...)
        {
            errorOut = "Batch sub-request count is not a number";
            return false;
        }
        cmd << "&NVars=" << parts[0];
        if (parts.size() != 1 + 2 * expected)
        {
            errorOut = "Batch sub-request count mismatch";
            return false;
        }
        for (size_t s = 0; s < expected; ++s)
        {
            const std::string &v = parts[1 + s * 2];
            const std::string &p = parts[2 + s * 2];
            cmd << "&Varname=" << UrlEncode(Base64urlDecode(v));
            if (!DecodePerRequestParamString(p, cmd))
            {
                errorOut =
                    "Malformed paramstring '" + p + "' in batch sub-request " + std::to_string(s);
                return false;
            }
        }
    }

    ssiCommandOut = cmd.str();
    return true;
}
} // namespace

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
  m_pathPrefix("/adios"), m_ssiLibPath(""), m_initialized(false)
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
    if (!verb || !path)
    {
        return false;
    }

    // Accept POST and GET
    if (strcmp(verb, "POST") != 0 && strcmp(verb, "GET") != 0)
    {
        return false;
    }

    // Match admin paths or prefix
    if (strncmp(path, "/_adios", 7) == 0)
    {
        return true;
    }
    if (strncmp(path, m_pathPrefix.c_str(), m_pathPrefix.length()) == 0)
    {
        return true;
    }
    // Legacy: accept POST at /ssi
    if (strcmp(verb, "POST") == 0 && strncmp(path, "/ssi", 4) == 0)
    {
        return true;
    }
    return false;
}

int XrdHttpSsiHandler::ProcessReq(XrdHttpExtReq &req)
{
    // Handle admin requests directly (no SSI needed)
    if (req.resource.compare(0, 7, "/_adios") == 0)
    {
        return ProcessAdminReq(req);
    }

    // Check if we're initialized
    if (!m_initialized || !m_ssiService)
    {
        return SendError(req, 503, "SSI service not available");
    }

    // Extract the resource name from the path (strip matching prefix)
    std::string resource = req.resource;
    if (resource.compare(0, m_pathPrefix.length(), m_pathPrefix) == 0)
    {
        resource = resource.substr(m_pathPrefix.length());
    }
    else if (resource.compare(0, 4, "/ssi") == 0)
    {
        resource = resource.substr(4); // legacy /ssi prefix
    }
    if (resource.empty())
    {
        resource = "/";
    }

    // The handler accepts two GET wire forms while old clients are
    // still being phased out:
    //   - path-encoded GET (new, cache-friendly):
    //       /adios/<filename>/<file-config>/<request>
    //     Distinguished by the absence of any '?' in the URL.
    //   - legacy query-string GET (old client form):
    //       /adios/<filename>?<verb>&<key>=<val>&...
    // POST is no longer supported (legacy POST builds are out of
    // circulation).
    if (req.verb != "GET")
    {
        return SendError(req, 405, "Only GET is supported");
    }

    // Use xrd-http-fullresource (the raw URI, unprocessed by XrdOucEnv)
    // so percent-encoded slashes in the filename segment survive intact
    // for path-encoded splits, and bare query tokens (e.g. `get` with
    // no `=`) survive for legacy parsing.
    auto it = req.headers.find("xrd-http-fullresource");
    if (it == req.headers.end() || it->second.empty())
    {
        return SendError(req, 400, "GET request missing xrd-http-fullresource");
    }

    std::string ssiCommand;
    if (it->second.find('?') == std::string::npos)
    {
        // No query string → new path-encoded form.
        std::string err;
        std::string pathResource;
        if (!DecodePathEncodedRequest(it->second, m_pathPrefix, ssiCommand, pathResource, err))
        {
            return SendError(req, 400, err.c_str());
        }
        // Use the filename from the path as the SSI resource so that
        // tar-archive resolution and caching key on the right path.
        resource = pathResource;
    }
    else
    {
        // Legacy query-string form.  The full URI has the shape
        // "<prefix>/<filename>?<verb>&<key>=<val>&...".  Split at '?'
        // to get verb-and-args; inject Filename= from the (decoded)
        // resource path.
        const std::string &raw = it->second;
        size_t qpos = raw.find('?');
        std::string query = raw.substr(qpos + 1);
        if (query.empty())
        {
            return SendError(req, 400, "GET request requires query parameters");
        }
        size_t ampPos = query.find('&');
        if (ampPos == std::string::npos)
        {
            ssiCommand = query; // verb only, no args
        }
        else
        {
            std::string verb = query.substr(0, ampPos);
            std::string args = query.substr(ampPos + 1);
            if (resource != "/")
            {
                ssiCommand = verb + " Filename=" + UrlEncode(resource) + "&" + args;
            }
            else
            {
                ssiCommand = verb + " " + args;
            }
        }
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
/*              A d m i n   E n d p o i n t   H a n d l e r                   */
/******************************************************************************/

// Function pointer type matching ADIOSPoolAdmin() in AdiosFilePool.cpp
typedef const char *(*AdminFunc)(const char *, const char *);

int XrdHttpSsiHandler::ProcessAdminReq(XrdHttpExtReq &req)
{
    // Resolve the admin function from the SSI plugin (loaded in same process)
    static AdminFunc adminFunc = nullptr;
    if (!adminFunc)
    {
        // Try the SSI library first (XRootD loads plugins with RTLD_LOCAL,
        // so dlopen(NULL) won't see their symbols)
        void *handle = nullptr;
        if (!m_ssiLibPath.empty())
        {
            handle = dlopen(m_ssiLibPath.c_str(), RTLD_NOW | RTLD_NOLOAD);
        }
        if (!handle)
        {
            handle = dlopen(NULL, RTLD_NOW);
        }
        if (handle)
        {
            adminFunc = (AdminFunc)dlsym(handle, "ADIOSPoolAdmin");
        }
    }
    if (!adminFunc)
    {
        return SendError(req, 503, "Admin interface not available (SSI plugin not loaded)");
    }

    // Extract command from path: /_adios/<command>
    std::string command;
    if (req.resource.length() > 8) // "/_adios/"
    {
        command = req.resource.substr(8);
    }

    // Get query string
    std::string query;
    auto it = req.headers.find("xrd-http-query");
    if (it != req.headers.end())
    {
        query = it->second;
    }

    if (command.empty())
    {
        std::string body = "Admin endpoints:\n"
                           "  GET /_adios/stats  - pool statistics (JSON)\n"
                           "  GET /_adios/files  - cached files (JSON)\n"
                           "  GET /_adios/flush  - flush idle cache entries\n"
                           "  GET /_adios/limits - view resource limits\n"
                           "  GET /_adios/limits?fd=N&md=N - set limits\n";
        return SendResponse(req, body.c_str(), body.size(), "text/plain");
    }

    const char *result = adminFunc(command.c_str(), query.c_str());
    return SendResponse(req, result, strlen(result), "application/json");
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
