/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.cpp
 *
 *  Created on: Apr 12, 2017
 *      Author: Jason Wang
 */

#include "utilities/realtime/dataman/DataMan.h"

namespace adios
{
namespace realtime
{

int DataMan::init(json p_jmsg) { return 0; }

int DataMan::put(const void *p_data, std::string p_doid, std::string p_var,
                 std::string p_dtype, std::vector<size_t> p_putshape,
                 std::vector<size_t> p_varshape, std::vector<size_t> p_offset,
                 size_t p_timestep, int p_tolerance, int p_priority)
{
    return DataMan::put(p_data, p_doid, p_var, p_dtype, p_putshape, p_varshape,
                        p_offset, p_timestep, p_tolerance, p_priority);
}

int DataMan::put(const void *p_data, json p_jmsg)
{
    put_begin(p_data, p_jmsg);
    put_end(p_data, p_jmsg);
    return 0;
}

void DataMan::add_file(std::string p_method) {}

void DataMan::add_stream(json p_jmsg)
{

    std::string method;

    if (p_jmsg["method"] != nullptr)
        method = p_jmsg["method"];

    logging("Streaming method " + method + " added");

    if (m_tolerance.size() < m_num_channels)
    {
        for (int i = 0; i < m_num_channels; i++)
        {
            m_tolerance.push_back(0);
        }
    }
    if (m_priority.size() < m_num_channels)
    {
        for (int i = 0; i < m_num_channels; i++)
        {
            m_priority.push_back(100 / (i + 1));
        }
    }

    auto man = get_man(method);
    if (man)
    {
        man->init(p_jmsg);
        this->add_next(method, man);
    }
    add_man_to_path("zfp", method);
}

void DataMan::flush() { flush_next(); }

int DataMan::get(void *p_data, json &p_jmsg) { return 0; }

// end namespace realtime
}
// end namespace adios
}
