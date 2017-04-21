/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ZmqMan.cpp
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#include "ZmqMan.h"

#include <sys/stat.h>
#include <unistd.h>

#include "zmq.h"

ZmqMan::~ZmqMan()
{
    if (zmq_data)
        zmq_close(zmq_data);
}

int ZmqMan::init(json p_jmsg)
{
    StreamMan::init(p_jmsg);
    zmq_data = zmq_socket(zmq_context, ZMQ_PAIR);
    std::string local_address =
        make_address(m_local_ip, m_local_port + 1, "tcp");
    std::string remote_address =
        make_address(m_remote_ip, m_remote_port + 1, "tcp");
    if (m_stream_mode == "sender")
    {
        zmq_connect(zmq_data, remote_address.c_str());
        logging("ZmqMan::init " + remote_address + " connected");
    }
    else if (m_stream_mode == "receiver")
    {
        zmq_bind(zmq_data, local_address.c_str());
        logging("ZmqMan::init " + local_address + " bound");
    }
    return 0;
}

int ZmqMan::put(const void *p_data, json p_jmsg)
{
    put_begin(p_data, p_jmsg);
    StreamMan::put(p_data, p_jmsg);
    zmq_send(zmq_data, p_data, p_jmsg["putbytes"], 0);
    put_end(p_data, p_jmsg);
    return 0;
}

int ZmqMan::get(void *p_data, json &p_jmsg) { return 0; }

void ZmqMan::on_recv(json msg)
{
    if (msg["operation"] == "put")
    {
        if (msg["compression_method"] == nullptr)
        {
            size_t putbytes = msg["putbytes"].get<size_t>();
            std::vector<char> data;
            data.resize(putbytes);
            int err = zmq_recv(zmq_data, data.data(), putbytes, 0);
            m_cache.put(data.data(), msg);
        }
        else
        {
            size_t putbytes = msg["putbytes"].get<size_t>();
            size_t compressed_size = msg["compressed_size"].get<size_t>();
            std::vector<char> compressed_data;
            compressed_data.resize(compressed_size);
            std::vector<char> data;
            data.resize(putbytes);
            int err =
                zmq_recv(zmq_data, compressed_data.data(), compressed_size, 0);
            auto_transform(compressed_data.data(), data.data(), msg);
            m_cache.put(data.data(), msg);
        }
    }
    else if (msg["operation"] == "flush")
    {
        callback();
        m_cache.flush();
        m_cache.clean_all("nan");
    }
}
