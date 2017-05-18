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
    virtual ~MdtmMan() = default;

    virtual int init(json a_jmsg);
    virtual int put(const void *a_data, json &a_jmsg);
    virtual int get(void *a_data, json &a_jmsg);
    virtual void transform(std::vector<char> &a_data, json &a_jmsg) {}

    virtual void on_recv(json &a_msg);
    virtual void on_put(std::shared_ptr<std::vector<char>> a_data);
    std::string name() { return "MdtmMan"; }

private:
    int zmq_msg_size = 1024;
    std::string m_pipepath = "/tmp/MdtmManPipes/";
    std::string m_pipename_prefix = "MdtmManPipe";
    std::string m_pipename;
    std::string m_full_pipename;
    int m_pipe_handler;
    std::string getmode = "callback";
    std::queue<json> jqueue;
    std::queue<std::vector<char>> vqueue;
    std::queue<int> iqueue;

}; // end of class MdtmMan

extern "C" DataManBase *getMan() { return new MdtmMan; }

#endif
