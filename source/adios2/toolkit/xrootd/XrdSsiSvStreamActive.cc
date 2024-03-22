/******************************************************************************/
/*                                                                            */
/*                 X r d S s i S t r e a m A c t i v e . c c                  */
/*                                                                            */
/* (c) 2014 by the Board of Trustees of the Leland Stanford, Jr., University  */
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

#include "XrdSsi/XrdSsiErrInfo.hh"

#include "XrdSsiSvStreamActive.hh"

using namespace std;

/******************************************************************************/
/*                         L o c a l   C l a s s e s                          */
/******************************************************************************/

// Here we define the actual implementation of the Buffer object that we need
// to return. It's very simple in that when it gets recycled it frees the data
// buffer and simply deletes itself. This is not every efficient and actual
// implementations would use reusable buffer pools and buffer objects to avoid
// constantly using new/delete/free.
//
class myBuffer : public XrdSsiStream::Buffer
{
public:
    void Recycle()
    {
        if (data)
            free(data);
        delete this;
    }

    myBuffer(char *buff) : Buffer(buff) {}
    ~myBuffer() {}
};

/******************************************************************************/
/*                               G e t B u f f                                */
/******************************************************************************/

XrdSsiStream::Buffer *XrdSsiSvStreamActive::GetBuff(XrdSsiErrInfo &eInfo, int &dlen, bool &last)
{
    myBuffer *sbP;
    char *buff;
    int buffsz;

    // Check if we should return an error (this occurs after the first successful
    // read so we can see if we get a buffer back).
    //
    if (rdSZ < 0)
    {
        if (didRead)
        {
            eInfo.Set(0, EIO);
            return 0;
        }
        else
            buffsz = -rdSZ;
    }
    else
        buffsz = (rdSZ ? rdSZ : dlen);
    didRead = true;

    // Normally we would have a buffer pool to avoid constant malloc and free
    // calls. But for simplicity we just allocate a buffer of the right size.
    //
    if (!(buff = (char *)malloc(buffsz)))
    {
        eInfo.Set(0, ENOMEM);
        return 0;
    }

    // Now get the data
    //
    if ((dlen = read(theFD, buff, buffsz)) < 0)
    {
        eInfo.Set(0, errno);
        free(buff);
        return 0;
    }

    // The last parameter is used to prevent needless calls and we set it if we
    // know that no more data remains. This is simple for us but may be more
    // complicated for unusual data sources.
    //
    last = (dlen < buffsz);

    // Now we allocate a buffer object. We would have a pool of those as well in
    // a real situtaion, but no need to be fancy here. Note that the buffer
    // object is meant to be inherited by data generators.
    //
    sbP = new myBuffer(buff);
    cerr << "Active stream GetBuff: " << hex << sbP << dec << " sz=" << dlen << endl;
    return sbP;
}
