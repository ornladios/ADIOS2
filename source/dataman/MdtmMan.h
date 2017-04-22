/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MdtmMan.h
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#ifndef DATAMAN_MDTMMAN_H_
#define DATAMAN_MDTMMAN_H_

#include "StreamMan.h"

#include <queue>

class MdtmMan : public StreamMan
{
public:
    MdtmMan() = default;
    virtual ~MdtmMan();

    virtual int init(json p_jmsg);
    virtual int put(const void *p_data, json p_jmsg);
    virtual int get(void *p_data, json &p_jmsg);
    virtual void transform(const void *p_in, void *p_out, json &p_jmsg){};

    void on_recv(json msg);
    std::string name() { return "MdtmMan"; }

private:
    void *zmq_ipc_req = NULL;
    int zmq_msg_size = 1024;
    std::string getmode = "callback";
    std::vector<int> pipes;
    std::vector<std::string> pipenames;
    std::queue<json> jqueue;
    std::queue<void *> bqueue;
    std::queue<int> iqueue;
    json pipe_desc;

}; // end of class MdtmMan

extern "C" DataManBase *getMan() { return new MdtmMan; }

#endif
