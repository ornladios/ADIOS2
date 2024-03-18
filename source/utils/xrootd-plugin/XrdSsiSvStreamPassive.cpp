/******************************************************************************/
/*                                                                            */
/*              X r d S s i S v S t r e a m P a s s i v e . c c               */
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

#include "XrdSsi/XrdSsiErrInfo.hh"

#include "XrdSsiSvStreamPassive.hh"

using namespace std;

/******************************************************************************/
/*                               G e t B u f f                                */
/******************************************************************************/

int XrdSsiSvStreamPassive::SetBuff(XrdSsiErrInfo &eInfo, char *buff, int blen, bool &last)
{
    int buffsz;

    // Check if we should return an error (this occurs after the first successful
    // read so we can see if we get a buffer back).
    //
    if (rdSZ < 0)
    {
        if (didRead)
        {
            eInfo.Set(0, EIO);
            return -1;
        }
        else
            buffsz = -rdSZ;
    }
    else
        buffsz = (rdSZ ? rdSZ : blen);
    if (rdSZ > blen)
        rdSZ = blen;
    didRead = true;

    // Now get the data
    //
    if ((blen = read(theFD, buff, buffsz)) < 0)
    {
        eInfo.Set(0, errno);
        return -1;
    }

    // The last parameter is used to prevent needless calls and we set it if we
    // know that no more data remains. This is simple for us but may be more
    // complicated for unusual data sources.
    //
    last = (blen < buffsz);

    // Now, we return the amount we actually read
    //
    cerr << "Passive stream SetBuff: sz=" << blen << endl;
    return blen;
}
