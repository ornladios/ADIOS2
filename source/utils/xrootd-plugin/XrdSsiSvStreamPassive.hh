#ifndef __XRDSSISVSTREAMPASSIVE_HH__
#define __XRDSSISVSTREAMPASSIVE_HH__
/******************************************************************************/
/*                                                                            */
/*              X r d S s i S v S t r e a m P a s s i v e . h h               */
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

#include <unistd.h>

#include "XrdSsi/XrdSsiStream.hh"

class XrdSsiErrInfo;

// This class displays how to use passive streams. Streams are good for
// streaming data out of a data source that can't give you the full response
// or one whose response requires some calculation. Passive streams are
// supplied a data buffer to fill with response data). They are very simple.
// Since this is for server-side use we need not define an async method as
// async streams are never used server-side.
//
class XrdSsiSvStreamPassive : public XrdSsiStream
{
public:
    // This is called to fill a buffer full of data that will be sent off
    //
    int SetBuff(XrdSsiErrInfo &eInfo, char *buff, int blen, bool &last);

    // The constructor we have accepts two arguments for testing puposes.

    // fd  - the file descriptor that will supply the data (we use a file).

    // rsz - tells us how much data the stream is actually to return which can be
    //       smaller or greater than the passed desired size. This will fully test
    //       the server-side stream handling logic and is immaterial to the example.
    //       If it is negative an error is returned after the first abs(rsz) amount
    //       is returned to test that code path as well.
    //
    XrdSsiSvStreamPassive(int fd, int rsz)
    : XrdSsiStream(isPassive), rdSZ(rsz), theFD(fd), didRead(false)
    {
    }

    ~XrdSsiSvStreamPassive() { close(theFD); }

private:
    int rdSZ;
    int theFD;
    bool didRead;
};
#endif
