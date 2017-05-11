/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ZmqMan.h
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#ifndef DATAMAN_ZMQMAN_H_
#define DATAMAN_ZMQMAN_H_

#include "StreamMan.h"

class ZmqMan : public StreamMan
{
public:
    ZmqMan() = default;
    virtual ~ZmqMan();

    virtual int init(json p_jmsg);
    virtual int put(const void *p_data, json p_jmsg);
    virtual int get(void *p_data, json &p_jmsg);
    virtual void transform(std::vector<char> &a_data, json &a_jmsg) {}

    virtual void on_recv(json msg);
    std::string name() { return "ZmqMan"; }

private:
    void *zmq_data = nullptr;
};

extern "C" DataManBase *getMan() { return new ZmqMan; }

#endif
