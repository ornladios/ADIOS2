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
    if (m_zmq_rep)
    {
        zmq_close(m_zmq_rep);
    }
    if (m_zmq_req)
    {
        zmq_close(m_zmq_req);
    }
    if (m_zmq_context)
    {
        zmq_ctx_destroy(m_zmq_context);
    }

    m_zmq_rep_thread_active = false;
    if (m_zmq_rep_thread.joinable())
    {
        m_zmq_rep_thread.join();
    }

    m_zmq_req_thread_active = false;
    if (m_zmq_req_thread.joinable())
    {
        m_zmq_req_thread.join();
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
        if (p_jmsg["tolerance"].is_number())
        {
            m_tolerance = p_jmsg["tolerance"].get<int>();
        }
        if (p_jmsg["priority"].is_number())
        {
            m_priority = p_jmsg["priority"].get<int>();
        }
        if (p_jmsg["num_channels"].is_number())
        {
            m_num_channels = p_jmsg["num_channels"].get<int>();
        }
        if (p_jmsg["channel_id"].is_number())
        {
            m_channel_id = p_jmsg["channel_id"].get<int>();
        }

        if (!m_zmq_context)
        {
            m_zmq_context = zmq_ctx_new();
            if (m_stream_mode == "sender")
            {
                m_zmq_req = zmq_socket(m_zmq_context, ZMQ_REQ);
                zmq_connect(m_zmq_req, remote_address.c_str());
                logging("Connecting " + remote_address + " ...");
            }
            else if (m_stream_mode == "receiver")
            {
                m_zmq_rep = zmq_socket(m_zmq_context, ZMQ_REP);
                zmq_bind(m_zmq_rep, local_address.c_str());
                logging("Binding " + local_address + " ...");
                m_zmq_rep_thread_active = true;
                m_zmq_rep_thread =
                    std::thread(&StreamMan::zmq_rep_thread_func, this);
            }
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

int StreamMan::callback_direct(const void *a_data, json &a_jmsg)
{
    if (!m_callback)
    {
        logging("callback called but callback function not registered!");
        return -1;
    }

    m_callback(a_data, a_jmsg["doid"], a_jmsg["var"],
               a_jmsg["dtype"].get<std::string>(),
               a_jmsg["varshape"].get<std::vector<size_t>>());
    return 0;
}

int StreamMan::callback_cache()
{
    if (!m_callback)
    {
        logging("callback called but callback function not registered!");
        return -1;
    }

    std::vector<std::string> do_list = m_cache.get_do_list();
    for (const std::string &i : do_list)
    {
        std::vector<std::string> var_list = m_cache.get_var_list(i);
        for (const std::string &j : var_list)
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
    char ret[10];
    zmq_send(m_zmq_req, msg.dump().c_str(), msg.dump().length(), 0);
    zmq_recv(m_zmq_req, ret, 10, 0);
}

void StreamMan::zmq_req_thread_func(std::shared_ptr<std::vector<char>> a_data)
{
    on_put(a_data);
}

void StreamMan::zmq_rep_thread_func()
{
    while (m_zmq_rep_thread_active)
    {
        char msg[1024] = "";
        int ret = zmq_recv(m_zmq_rep, msg, 1024, ZMQ_NOBLOCK);
        zmq_send(m_zmq_rep, "OK", 4, 0);
        std::string smsg = msg;
        if (ret >= 0)
        {
            json jmsg = json::parse(msg);
            logging("StreamMan::zmq_meta_rep_thread_func: \n" + jmsg.dump(4));
            on_recv(jmsg);
        }
        else
        {
            usleep(1);
        }
    }
}

int StreamMan::put_stream(const void *a_data, json a_jmsg)
{
    if (m_zmq_req_thread.joinable())
    {
        m_zmq_req_thread.join();
    }
    a_jmsg["operation"] = "put";
    char ret[10];
    zmq_send(m_zmq_req, a_jmsg.dump().c_str(), a_jmsg.dump().length(), 0);
    zmq_recv(m_zmq_req, ret, 10, 0);

    // copy
    std::shared_ptr<std::vector<char>> data =
        std::make_shared<std::vector<char>>();
    data->resize(a_jmsg["sendbytes"].get<size_t>());
    std::memcpy(data->data(), a_data, a_jmsg["sendbytes"].get<size_t>());

    m_zmq_req_thread = std::thread(&StreamMan::zmq_req_thread_func, this, data);
    return 0;
}
