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

MdtmMan::~MdtmMan()
{
    if (zmq_ipc_req)
    {
        zmq_close(zmq_ipc_req);
    }
}

int MdtmMan::init(json p_jmsg)
{

    StreamMan::init(p_jmsg);

    if (p_jmsg["pipe_prefix"].is_string())
    {
        pipe_desc["pipe_prefix"] = p_jmsg["pipe_prefix"].get<std::string>();
    }
    else
    {
        pipe_desc["pipe_prefix"] = "/tmp/MdtmManPipes/";
    }

    pipe_desc["operation"] = "init";
    pipe_desc["mode"] = m_stream_mode;

    std::string pipename_prefix = "MdtmManPipe";
    for (int i = 0; i < m_num_channels; ++i)
    {
        std::stringstream pipename;
        pipename << pipename_prefix << i;
        if (i == 0)
        {
            pipe_desc["pipe_names"] = {pipename.str()};
            pipe_desc["loss_tolerance"] = {m_tolerance[i]};
            pipe_desc["priority"] = {m_priority[i]};
        }
        else
        {
            pipe_desc["pipe_names"].insert(pipe_desc["pipe_names"].end(),
                                           pipename.str());
            pipe_desc["loss_tolerance"].insert(
                pipe_desc["loss_tolerance"].end(), m_tolerance[i]);
            pipe_desc["priority"].insert(pipe_desc["priority"].end(),
                                         m_priority[i]);
        }
    }

    // ZMQ_DataMan_MDTM
    if (m_stream_mode == "sender")
    {
        zmq_ipc_req = zmq_socket(zmq_context, ZMQ_REQ);
        zmq_connect(zmq_ipc_req, "ipc:///tmp/ADIOS_MDTM_pipe");
        char buffer_return[10];
        zmq_send(zmq_ipc_req, pipe_desc.dump().c_str(),
                 pipe_desc.dump().length(), 0);
        zmq_recv(zmq_ipc_req, buffer_return, sizeof(buffer_return), 0);
    }

    // Pipes
    mkdir(pipe_desc["pipe_prefix"].get<std::string>().c_str(), 0755);
    for (const auto &i :
         pipe_desc["pipe_names"].get<std::vector<std::string>>())
    {
        std::string filename = pipe_desc["pipe_prefix"].get<std::string>() + i;
        mkfifo(filename.c_str(), 0666);
    }

    for (int i = 0; i < m_num_channels; ++i)
    {
        std::stringstream pipename;
        pipename << pipename_prefix << i;
        std::string fullpipename =
            pipe_desc["pipe_prefix"].get<std::string>() + pipename.str();
        if (m_stream_mode == "sender")
        {
            int fp = open(fullpipename.c_str(), O_WRONLY);
            pipes.push_back(fp);
        }
        if (m_stream_mode == "receiver")
        {
            int fp = open(fullpipename.c_str(), O_RDONLY | O_NONBLOCK);
            pipes.push_back(fp);
        }
        pipenames.push_back(pipename.str());
    }
    return 0;
}

int MdtmMan::put(const void *a_data, json a_jmsg)
{
    put_begin(a_data, a_jmsg);

    // determine pipe to use
    int priority = 100;
    if (a_jmsg["priority"].is_number_integer())
    {
        priority = a_jmsg["priority"].get<int>();
    }
    int index;
    if (m_parallel_mode == "round")
    {
        if (m_current_channel == m_num_channels - 1)
        {
            index = 0;
            m_current_channel = 0;
        }
        else
        {
            m_current_channel++;
            index = m_current_channel;
        }
    }
    else if (m_parallel_mode == "priority")
    {
        index = closest(priority, pipe_desc["priority"], true);
    }
    a_jmsg["pipe"] = pipe_desc["pipe_names"][index];

    StreamMan::put(a_data, a_jmsg);
    size_t sendbytes = a_jmsg["sendbytes"].get<size_t>();
    write(pipes[index], a_data, sendbytes);
    put_end(a_data, a_jmsg);
    return 0;
}

int MdtmMan::get(void *p_data, json &p_jmsg) { return 0; }

void MdtmMan::on_recv(json a_jmsg)
{

    // push new request
    jqueue.push(a_jmsg);
    vqueue.push(std::vector<char>());
    iqueue.push(0);

    // for flush
    if (jqueue.front()["operation"] == "flush")
    {
        callback();
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

            // determine the pipe for the head request
            if (jmsg == nullptr)
            {
                break;
            }
            int pipeindex = 0;
            for (int i = 0; i < pipenames.size(); ++i)
            {
                if (jmsg["pipe"].get<std::string>() == pipenames[i])
                {
                    pipeindex = i;
                }
            }

            // read the head request
            int error_times = 0;
            while (iqueue.front() < sendbytes)
            {
                int ret = read(pipes[pipeindex],
                               vqueue.front().data() + iqueue.front(),
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
                m_cache.put(vqueue.front().data(), jmsg);
                jqueue.pop();
                vqueue.pop();
                iqueue.pop();
                break;
            }
        }
    }
}
