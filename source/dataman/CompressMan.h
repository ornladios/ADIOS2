/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressMan.h
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#ifndef COMPRESSMAN_H_
#define COMPRESSMAN_H_

#include "DataMan.h"

class CompressMan : public DataManBase
{
public:
    CompressMan() = default;
    ~CompressMan() = default;
    virtual std::string type() { return "Compress"; }
};

#endif
