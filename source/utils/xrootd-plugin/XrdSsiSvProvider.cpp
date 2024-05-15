/******************************************************************************/
/*                                                                            */
/*                    X r d S s i S v S e r v i c e . c c                     */
/*                                                                            */
/* (c) 2013 by the Board of Trustees of the Leland Stanford, Jr., University  */
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

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "XrdNet/XrdNetAddr.hh"

#include "XrdSsi/XrdSsiCluster.hh"
#include "XrdSsi/XrdSsiLogger.hh"
#include "XrdSsi/XrdSsiProvider.hh"
#include "XrdSsi/XrdSsiResource.hh"

#include "XrdSsiSvService.hh"

/******************************************************************************/
/*                               D e f i n e s                                */
/******************************************************************************/

#define TRACE(x) std::cerr << "Prep: " << x << std::endl

/******************************************************************************/
/*                         L o c a l   C l a s s e s                          */
/******************************************************************************/

class XrdSsiSvProvider : public XrdSsiProvider
{
public:
    XrdSsiService *GetService(XrdSsiErrInfo &eInfo, const std::string &contact, int oHold = 256)
    {
        return new XrdSsiSvService;
    }

    virtual bool Init(XrdSsiLogger *logP, XrdSsiCluster *clsP, std::string cfgFn, std::string parms,
                      int argc, char **argv);

    virtual rStat QueryResource(const char *rName, const char *contact = 0)
    {
        if (!strcmp(rName, "/tmp/none"))
            return notPresent;
        if (!strcmp(rName, "/tmp/pend"))
            return isPending;
        return isPresent;
    }

    XrdSsiSvProvider() {}
    virtual ~XrdSsiSvProvider() {}
};

/******************************************************************************/
/*                               G l o b a l s                                */
/******************************************************************************/

namespace XrdSsiSv
{
XrdSsiLogger *mDest;       // Object to use for mesages
XrdSsiCluster *Cluster;    // Object to use to control the cluster
XrdNetAddr myAddr((int)0); // A way to get our host name for testing
XrdSsiSvProvider SvProvider;
}

XrdSsiProvider *XrdSsiProviderLookup = &XrdSsiSv::SvProvider;

XrdSsiProvider *XrdSsiProviderServer = &XrdSsiSv::SvProvider;

/******************************************************************************/
/*                L o g g i n g   I n t e r c e p t   T e s t                 */
/******************************************************************************/

namespace
{
void LogMsg(struct timeval const &mtime, unsigned long tID, const char *msg, int mlen)
{
    if (mtime.tv_sec)
        std::cout << "ssi: " << tID << ' ' << msg << std::flush;
    else
        std::cout << "ssi: " << msg << std::flush;
}
}

XrdSsiLogger::MCB_t *XrdSsiLoggerMCB = &LogMsg;

/******************************************************************************/
/*                X r d S s i S v P r o v i d e r : : I n i t                 */
/******************************************************************************/

bool XrdSsiSvProvider::Init(XrdSsiLogger *logP, XrdSsiCluster *clsP, std::string cfgFn,
                            std::string parms, int argc, char **argv)
{
    // We don't process a configuration file nor have any parameters, so we will
    // ingnore those two parameters. We will, however, record the log and cluster
    // pointers in a globally accessible area in our own namespace. Then return
    // success!
    //
    XrdSsiSv::mDest = logP;
    XrdSsiSv::Cluster = clsP;
    return true;
}
