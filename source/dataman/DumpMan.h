/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DumpMan.h
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#ifndef DUMPMAN_H_
#define DUMPMAN_H_

#include "DataMan.h"

class DumpMan : public DataManBase
{
public:
    DumpMan() = default;
    virtual ~DumpMan() = default;

    virtual int init(json p_jmsg);
    virtual int put(const void *p_data, json p_jmsg);
    virtual int get(void *p_data, json &p_jmsg);
    void flush();
    std::string name() { return "DumpMan"; }
    std::string type() { return "Dump"; }
    virtual void transform(const void *p_in, void *p_out, json &p_jmsg){};

private:
    bool m_dumping = true;
};

extern "C" DataManBase *getMan() { return new DumpMan; }

#endif
