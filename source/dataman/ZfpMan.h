/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ZfpMan.h
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#ifndef DATAMAN_ZFPMAN_H_
#define DATAMAN_ZFPMAN_H_

#include "CompressMan.h"

class ZfpMan : public CompressMan
{
public:
    ZfpMan() = default;
    virtual ~ZfpMan() = default;
    virtual int init(json p_jmsg);
    virtual int put(const void *p_data, json p_jmsg);
    virtual int get(void *p_data, json &p_jmsg);
    virtual void flush();
    void *compress(void *p_data, json &p_jmsg);
    void *decompress(void *p_data, json p_jmsg);
    virtual void transform(const void *p_in, void *p_out, json &p_jmsg);
    std::string name() { return "ZfpMan"; }
private:
    double m_compression_rate = 8;
};

extern "C" DataManBase *getMan() { return new ZfpMan; }

#endif
