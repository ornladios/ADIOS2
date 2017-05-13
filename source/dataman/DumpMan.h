/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DumpMan.h
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#ifndef DATAMAN_DUMPMAN_H_
#define DATAMAN_DUMPMAN_H_

#include "DataMan.h"

class DumpMan : public DataManBase
{
public:
    DumpMan() = default;
    virtual ~DumpMan() = default;

    virtual int init(json a_jmsg);
    virtual int put(const void *a_data, json &a_jmsg);
    virtual int get(void *a_data, json &a_jmsg);
    void flush();
    std::string name() { return "DumpMan"; }
    std::string type() { return "Dump"; }
    virtual void transform(std::vector<char> &a_data, json &a_jmsg) {}

private:
    bool m_dumping = true;
};

extern "C" DataManBase *getMan() { return new DumpMan; }

#endif
