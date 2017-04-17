/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.h
 *
 *  Created on: Apr 12, 2017
 *      Author: Jason Wang
 */

#ifndef DATAMANAGER_H_
#define DATAMANAGER_H_

#include "utilities/realtime/dataman/DataManBase.h"

namespace adios
{
namespace realtime
{

class DataMan : public DataManBase
{
public:
    DataMan() = default;
    virtual ~DataMan() = default;
    virtual int init(json p_jmsg);
    virtual int put(const void *p_data, json p_jmsg);
    virtual int get(void *p_data, json &p_jmsg);
    void flush();
    void add_stream(json p_jmsg);
    int put(const void *p_data, std::string p_doid, std::string p_var,
            std::string p_dtype, std::vector<size_t> p_putshape,
            std::vector<size_t> p_varshape, std::vector<size_t> p_offset,
            size_t p_timestep, int p_tolerance = 0, int p_priority = 100);
    void add_file(std::string p_method);
    std::string name() { return "DataManager"; }
    std::string type() { return "Manager"; }
    virtual void transform(const void *p_in, void *p_out, json &p_jmsg){};

private:
    std::string m_local_ip = "";
    std::string m_remote_ip = "";
    int m_local_port = 0;
    int m_remote_port = 0;
    int m_num_channels = 0;
    std::vector<int> m_tolerance;
    std::vector<int> m_priority;
};

// end namespace realtime
}
// end namespace adios
}
#endif
