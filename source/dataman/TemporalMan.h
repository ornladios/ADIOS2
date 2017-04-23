/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TemporalMan.h
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#ifndef DATAMAN_TEMPORALMAN_H_
#define DATAMAN_TEMPORALMAN_H_

#include "CompressMan.h"

class TemporalMan : public CompressMan
{
public:
    TemporalMan() = default;
    virtual ~TemporalMan() = default;
    virtual int init(json p_jmsg);
    virtual int put(const void *p_data, json p_jmsg);
    virtual int get(void *p_data, json &p_jmsg);
    virtual void flush();
    virtual void transform(std::vector<char> &a_data, json &a_jmsg);
    std::string name() { return "TemporalMan"; }
};

extern "C" DataManBase *getMan() { return new TemporalMan; }

#endif
