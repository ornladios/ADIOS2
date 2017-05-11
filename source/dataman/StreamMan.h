/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StreamMan.h
 *
 *  Created on: Apr 20, 2017
 *      Author: Jason Wang
 */

#ifndef DATAMAN_STREAMMAN_H_
#define DATAMAN_STREAMMAN_H_

#include "CacheMan.h"
#include "DataManBase.h"

#include <thread>

class StreamMan : public DataManBase
{
public:
    StreamMan() = default;
    virtual ~StreamMan();

    virtual int init(json p_jmsg);
    virtual int put(const void *p_data, json p_jmsg);
    virtual void on_recv(json msg) = 0;
    void flush();
    virtual std::string type() { return "Stream"; }

protected:
    void *zmq_context = NULL;
    CacheMan m_cache;
    int callback_direct(const void *a_data, json &a_jmsg);
    int callback_cache();

    std::string m_get_mode = "callback";
    std::string m_stream_mode;
    std::string m_local_ip;
    std::string m_remote_ip;
    int m_local_port;
    int m_remote_port;
    int m_num_channels = 10;
    std::vector<int> m_tolerance;
    std::vector<int> m_priority;
    std::string m_clean_mode = "nan";

    // parallel
    std::string m_parallel_mode = "round"; // round, priority
    int m_current_channel = 0;

    inline std::string make_address(std::string ip, int port,
                                    std::string protocol)
    {
        std::stringstream address;
        address << protocol << "://" << ip << ":" << port;
        return address.str();
    }

private:
    void *zmq_meta = NULL;
    void zmq_meta_rep_thread_func();
    bool zmq_meta_rep_thread_active;
    std::thread zmq_meta_rep_thread;
};

#endif
