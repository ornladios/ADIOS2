#ifndef __XRDSSISVSTREAMACTIVE_HH__
#define __XRDSSISVSTREAMACTIVE_HH__
/******************************************************************************/
/*                                                                            */
/*                 X r d S s i S t r e a m A c t i v e . h h                  */
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

// This class displays how to use active streams. Streams are good for
// streaming data out of a data source that can't give you the full response
// or one whose response requires some calculation. Active streams supply
// a data buffer to the caller (passive streams fill a supplied data buffer).
// They are good to use when the data source manages the data buffers.
//
class XrdSsiSvStreamActive : public XrdSsiStream
{
public:
    // This is called to get a buffer full of data that will be sent off
    //
    Buffer *GetBuff(XrdSsiErrInfo &eInfo, int &dlen, bool &last);

    // The constructor we have accepts two arguments for testing puposes.

    // fd  - the file descriptor that will supply the data (we use a file).

    // rsz - tells us how much data the stream is actually to return which can be
    //       smaller or greater than the passed desired size. This will fully test
    //       the server-side stream handling logic and is immaterial to the example.
    //       If it is negative an error is returned after the first abs(rsz) amount
    //       is returned to test that code path as well.
    //
    XrdSsiSvStreamActive(int fd, int rsz)
    : XrdSsiStream(isActive), rdSZ(rsz), theFD(fd), didRead(false)
    {
    }

    ~XrdSsiSvStreamActive() { close(theFD); }

private:
    int rdSZ;
    int theFD;
    bool didRead;
};
#endif
