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

int ZmqMan::init(json a_jmsg)
{
    StreamMan::init(a_jmsg);
    if (m_stream_mode == "sender")
    {
        zmq_data = zmq_socket(zmq_context, ZMQ_REQ);
        std::string remote_address =
            make_address(m_remote_ip, m_remote_port + 1, "tcp");
        zmq_connect(zmq_data, remote_address.c_str());
        logging("ZmqMan::init " + remote_address + " connected");
    }
    else if (m_stream_mode == "receiver")
    {
        zmq_data = zmq_socket(zmq_context, ZMQ_REP);
        std::string local_address =
            make_address(m_local_ip, m_local_port + 1, "tcp");
        zmq_bind(zmq_data, local_address.c_str());
        logging("ZmqMan::init " + local_address + " bound");
    }
    return 0;
}

int ZmqMan::put(const void *a_data, json a_jmsg)
{
    char ret[10];
    DataManBase::put_begin(a_data, a_jmsg);
    StreamMan::put(a_data, a_jmsg);
    zmq_send(zmq_data, a_data, a_jmsg["sendbytes"].get<size_t>(), 0);
    zmq_recv(zmq_data, ret, 10, 0);
    DataManBase::put_end(a_data, a_jmsg);
    return 0;
}

int ZmqMan::get(void *a_data, json &a_jmsg) { return 0; }

void ZmqMan::on_recv(json a_jmsg)
{
    if (a_jmsg["operation"].get<std::string>() == "put")
    {
        size_t sendbytes = a_jmsg["sendbytes"].get<size_t>();
        std::vector<char> data(sendbytes);
        int ret = zmq_recv(zmq_data, data.data(), sendbytes, 0);
        zmq_send(zmq_data, "OK", 10, 0);

        // if data is compressed then call auto_transform to decompress
        if (a_jmsg["compression_method"].is_string())
        {
            if (a_jmsg["compression_method"].get<std::string>() != "null")
            {
                auto_transform(data, a_jmsg);
            }
        }

        if (a_jmsg["varshape"] == a_jmsg["putshape"])
        {
            callback_direct(data.data(), a_jmsg);
        }
        else
        {
            m_cache.put(data.data(), a_jmsg);
        }
    }
    else if (a_jmsg["operation"].get<std::string>() == "flush")
    {
        callback_cache();
    }
}
