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

    virtual int init(json a_jmsg);
    virtual void on_recv(json &a_msg) = 0;
    virtual void on_put(std::shared_ptr<std::vector<char>> a_data) = 0;
    void flush();
    virtual std::string type() { return "Stream"; }

protected:
    int callback_direct(const void *a_data, json &a_jmsg);
    int callback_cache();
    int put_stream(const void *a_data, json a_jmsg);

    void *m_zmq_context = nullptr;
    CacheMan m_cache;
    std::string m_get_mode = "callback";
    std::string m_stream_mode;
    std::string m_local_ip;
    std::string m_remote_ip;
    int m_local_port;
    int m_remote_port;
    int m_tolerance = 0;
    int m_priority = 100;
    int m_channel_id = 0;
    int m_num_channels = 1;
    std::string m_clean_mode = "nan";

    inline std::string make_address(std::string ip, int port,
                                    std::string protocol)
    {
        std::stringstream address;
        address << protocol << "://" << ip << ":" << port;
        return address.str();
    }

private:
    void zmq_rep_thread_func();
    std::thread m_zmq_rep_thread;
    void *m_zmq_rep = nullptr;
    bool m_zmq_rep_thread_active = false;

    void zmq_req_thread_func(std::shared_ptr<std::vector<char>> a_data);
    std::thread m_zmq_req_thread;
    void *m_zmq_req = nullptr;
    bool m_zmq_req_thread_active = false;
};

#endif
