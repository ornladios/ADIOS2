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
    virtual int init(json a_jmsg);
    virtual int put(const void *a_data, json a_jmsg);
    virtual int get(void *a_data, json &a_jmsg);
    virtual void transform(std::vector<char> &a_data, json &a_jmsg);
    virtual void flush();
    int compress(void *a_input, std::vector<char> &a_output, json &a_jmsg);
    int decompress(void *a_input, std::vector<char> &a_output, json &a_jmsg);
    std::string name() { return "ZfpMan"; }
private:
    double m_compression_rate = 8;
};

extern "C" DataManBase *getMan() { return new ZfpMan; }

#endif
