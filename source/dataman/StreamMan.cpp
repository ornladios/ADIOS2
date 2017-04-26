/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StreamMan.cpp
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#include "StreamMan.h"

#include <unistd.h>

#include <iostream>
#include <sstream>

#include "zmq.h"

StreamMan::~StreamMan()
{
    if (zmq_meta)
    {
        zmq_close(zmq_meta);
    }
    if (zmq_context)
    {
        zmq_ctx_destroy(zmq_context);
    }
    zmq_meta_rep_thread_active = false;
    if (zmq_meta_rep_thread.joinable())
    {
        zmq_meta_rep_thread.join();
    }
}

int StreamMan::init(json p_jmsg)
{
    if (check_json(p_jmsg, {"stream_mode", "remote_ip", "local_ip",
                            "remote_port", "local_port"},
                   "StreamMan"))
    {
        m_stream_mode = p_jmsg["stream_mode"];
        m_local_ip = p_jmsg["local_ip"];
        m_remote_ip = p_jmsg["remote_ip"];
        m_local_port = p_jmsg["local_port"];
        m_remote_port = p_jmsg["remote_port"];
        std::string remote_address =
            make_address(m_remote_ip, m_remote_port, "tcp");
        std::string local_address =
            make_address(m_local_ip, m_local_port, "tcp");

        if (p_jmsg["clean_mode"].is_string())
        {
            m_clean_mode = p_jmsg["clean_mode"];
        }
        m_tolerance.assign(m_num_channels, 0);
        m_priority.assign(m_num_channels, 100);
        if (p_jmsg["num_channels"].is_number_integer())
        {
            m_num_channels = p_jmsg["num_channels"].get<int>();
        }
        if (p_jmsg["tolerance"] != nullptr)
        {
            m_tolerance = p_jmsg["tolerance"].get<std::vector<int>>();
        }
        if (p_jmsg["priority"] != nullptr)
        {
            m_priority = p_jmsg["priority"].get<std::vector<int>>();
        }

        if (!zmq_context)
        {
            zmq_context = zmq_ctx_new();
            zmq_meta = zmq_socket(zmq_context, ZMQ_PAIR);
            if (m_stream_mode == "sender")
            {
                zmq_connect(zmq_meta, remote_address.c_str());
                logging("StreamMan::init " + remote_address + " connected");
            }
            else if (m_stream_mode == "receiver")
            {
                zmq_bind(zmq_meta, local_address.c_str());
                logging("StreamMan::init " + local_address + " bound");
            }
            zmq_meta_rep_thread_active = true;
            zmq_meta_rep_thread =
                std::thread(&StreamMan::zmq_meta_rep_thread_func, this);
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

int StreamMan::callback()
{
    if (!m_callback)
    {
        logging("callback called but callback function not registered!");
        return -1;
    }

    std::vector<std::string> do_list = m_cache.get_do_list();
    for (std::string i : do_list)
    {
        std::vector<std::string> var_list = m_cache.get_var_list(i);
        for (std::string j : var_list)
        {
            m_callback(
                m_cache.get(i, j), i, j,
                m_cache.get_jmsg(i, j)["dtype"].get<std::string>(),
                m_cache.get_jmsg(i, j)["varshape"].get<std::vector<size_t>>());
        }
    }
    m_cache.clean("nan");
    return 0;
}

void StreamMan::flush()
{
    json msg;
    msg["operation"] = "flush";
    zmq_send(zmq_meta, msg.dump().c_str(), msg.dump().length(), 0);
}

void StreamMan::zmq_meta_rep_thread_func()
{
    while (zmq_meta_rep_thread_active)
    {
        char msg[1024] = "";
        int ret = zmq_recv(zmq_meta, msg, 1024, ZMQ_NOBLOCK);
        std::string smsg = msg;
        if (ret >= 0)
        {
            json jmsg = json::parse(msg);
            logging("StreamMan::zmq_meta_rep_thread_func: \n" + jmsg.dump(4));
            on_recv(jmsg);
        }
        usleep(10);
    }
}

int StreamMan::put(const void *p_data, json p_jmsg)
{
    p_jmsg["operation"] = "put";
    zmq_send(zmq_meta, p_jmsg.dump().c_str(), p_jmsg.dump().length(), 0);
    return 0;
}



