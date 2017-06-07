/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.cpp
 *
 *  Created on: Apr 12, 2017
 *      Author: Jason Wang
 */

#include "DataMan.h"

int DataMan::init(json a_jmsg) { return 0; }

int DataMan::put_streams(const void *a_data, json &a_jmsg)
{
    a_jmsg["channel_id"] = m_stream_index;
    m_stream_mans[m_stream_index]->put(a_data, a_jmsg);
    ++m_stream_index;
    if (m_stream_index >= m_stream_mans.size())
    {
        m_stream_index = 0;
    }
    return 0;
}

int DataMan::put(const void *a_data, json &a_jmsg)
{
    a_jmsg["timestep"] = m_timestep;
    if (m_cache_size > 0)
    {
        check_shape(a_jmsg);
        m_cache.put(a_data, a_jmsg);
    }
    else
    {
        put_begin(a_data, a_jmsg);
        put_streams(a_data, a_jmsg);
        put_end(a_data, a_jmsg);
    }
    dump_profiling();
    return 0;
}

void DataMan::add_file(std::string p_method) {}

void DataMan::add_stream(json a_jmsg)
{

    std::string method;

    if (a_jmsg["method"].is_string())
    {
        method = a_jmsg["method"];
    }

    logging("Streaming method " + method + " added");

    if (a_jmsg["cachesize"].is_number())
    {
        m_cache_size = a_jmsg["cachesize"].get<size_t>();
    }

    int num_channels = 1;

    if (a_jmsg["num_channels"].is_number())
    {
        num_channels = a_jmsg["num_channels"].get<int>();
    }
    else
    {
        a_jmsg["num_channels"] = num_channels;
    }

    int local_port = 12306, remote_port = 12307;

    if (a_jmsg["local_port"].is_number())
    {
        local_port = a_jmsg["local_port"].get<int>();
    }

    if (a_jmsg["remote_port"].is_number())
    {
        local_port = a_jmsg["remote_port"].get<int>();
    }

    for (int i = 0; i < num_channels; i++)
    {
        a_jmsg["channel_id"] = i;
        a_jmsg["local_port"] = local_port + 2;
        a_jmsg["remote_port"] = remote_port + 2;
        auto man = get_man(method);
        if (man)
        {
            std::cout << a_jmsg.dump(4);
            man->init(a_jmsg);
            m_stream_mans.push_back(man);
        }
        if (a_jmsg["compression_method"].is_string())
        {
            if (a_jmsg["compression_method"] != "null")
            {
            }
        }
    }
}

void DataMan::flush()
{
    m_timestep++;
    if (m_cache_size > 0)
    {
        if (m_cache_size == m_cache.get_timesteps_cached())
        {
            for (int i = 0; i < m_cache_size; ++i)
            {
                std::vector<std::string> do_list = m_cache.get_do_list();
                for (const auto &j : do_list)
                {
                    std::vector<std::string> var_list = m_cache.get_var_list(j);
                    for (const auto &k : var_list)
                    {
                        json jmsg = m_cache.get_jmsg(j, k);
                        put_begin(m_cache.get(j, k), jmsg);
                        put_streams(m_cache.get(j, k), jmsg);
                        put_end(m_cache.get(j, k), jmsg);
                    }
                }
                m_cache.pop();
            }
        }
        else
        {
            m_cache.push();
        }
    }
}

int DataMan::get(void *a_data, json &a_jmsg) { return 0; }
