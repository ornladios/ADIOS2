/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MdtmMan.cpp
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#include "MdtmMan.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <zmq.h>

int MdtmMan::init(json a_jmsg)
{

    StreamMan::init(a_jmsg);

    if (a_jmsg["pipe_prefix"].is_string())
    {
        m_pipepath = a_jmsg["pipe_prefix"].get<std::string>();
    }

    json pipe_desc;
    pipe_desc["operation"] = "init";
    pipe_desc["pipe_prefix"] = m_pipepath;
    pipe_desc["mode"] = m_stream_mode;

    std::stringstream pname;
    pname << m_pipename_prefix << m_channel_id;
    m_pipename = pname.str();
    m_full_pipename = m_pipepath + m_pipename;

    // send JSON message to MDTM
    if (m_channel_id == 0)
    {
        for (int i = 0; i < m_num_channels; ++i)
        {
            std::stringstream pipename;
            pipename << m_pipename_prefix << i;
            if (i == 0)
            {
                pipe_desc["pipe_names"] = {pipename.str()};
            }
            else
            {
                pipe_desc["pipe_names"].insert(pipe_desc["pipe_names"].end(),
                                               pipename.str());
            }
        }
        void *zmq_ipc_req = nullptr;
        zmq_ipc_req = zmq_socket(m_zmq_context, ZMQ_REQ);
        zmq_connect(zmq_ipc_req, "ipc:///tmp/ADIOS_MDTM_pipe");
        char buffer_return[10];
        zmq_send(zmq_ipc_req, pipe_desc.dump().c_str(),
                 pipe_desc.dump().length(), 0);
        zmq_recv(zmq_ipc_req, buffer_return, sizeof(buffer_return), 0);
        if (zmq_ipc_req)
        {
            zmq_close(zmq_ipc_req);
        }
    }

    // Make pipes
    mkdir(m_pipepath.c_str(), 0755);
    mkfifo(m_full_pipename.c_str(), 0666);

    if (m_stream_mode == "sender")
    {
        m_pipe_handler = open(m_full_pipename.c_str(), O_WRONLY);
    }
    if (m_stream_mode == "receiver")
    {
        m_pipe_handler = open(m_full_pipename.c_str(), O_RDONLY | O_NONBLOCK);
    }
    return 0;
}

int MdtmMan::put(const void *a_data, json &a_jmsg)
{
    a_jmsg["pipe"] = m_pipename;
    put_begin(a_data, a_jmsg);
    StreamMan::put_stream(a_data, a_jmsg);
    put_end(a_data, a_jmsg);
    return 0;
}

int MdtmMan::get(void *a_data, json &a_jmsg) { return 0; }

void MdtmMan::on_put(std::shared_ptr<std::vector<char>> a_data)
{
    write(m_pipe_handler, a_data->data(), a_data->size());
}

void MdtmMan::on_recv(json &a_jmsg)
{

    // push new request
    jqueue.push(a_jmsg);
    vqueue.push(std::vector<char>());
    iqueue.push(0);

    // for flush
    if (jqueue.front()["operation"] == "flush")
    {
        callback_cache();
        jqueue.pop();
        vqueue.pop();
        iqueue.pop();
    }

    if (jqueue.empty())
    {
        return;
    }

    // for put
    for (int outloop = 0; outloop < jqueue.size() * 2; outloop++)
    {
        if (jqueue.front()["operation"] == "put")
        {
            json &jmsg = jqueue.front();

            // allocate buffer
            size_t sendbytes = jmsg["sendbytes"].get<size_t>();
            vqueue.front() = std::vector<char>(sendbytes);

            // read the head request
            int error_times = 0;
            while (iqueue.front() < sendbytes)
            {
                int ret =
                    read(m_pipe_handler, vqueue.front().data() + iqueue.front(),
                         sendbytes - iqueue.front());
                if (ret > 0)
                {
                    iqueue.front() += ret;
                }
                else
                {
                    error_times++;
                    continue;
                }
                if (error_times > 1000000)
                {
                    break;
                }
            }

            if (iqueue.front() == sendbytes)
            {
                if (a_jmsg["compression_method"].is_string())
                {
                    if (a_jmsg["compression_method"].get<std::string>() !=
                        "null")
                    {
                        auto_transform(vqueue.front(), a_jmsg);
                    }
                }

                if (a_jmsg["varshape"] == a_jmsg["putshape"])
                {
                    std::cout << "callback_direct \n";
                    callback_direct(vqueue.front().data(), jmsg);
                }
                else
                {
                    m_cache.put(vqueue.front().data(), jmsg);
                }

                jqueue.pop();
                vqueue.pop();
                iqueue.pop();
                break;
            }
        }
    }
}
