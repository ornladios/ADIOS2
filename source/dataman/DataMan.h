/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.h
 *
 *  Created on: Apr 12, 2017
 *      Author: Jason Wang
 */

#ifndef DATAMAN_DATAMAN_H_
#define DATAMAN_DATAMAN_H_

#include "CacheMan.h"
#include "DataManBase.h"

class DataMan : public DataManBase
{
public:
    DataMan() = default;
    virtual ~DataMan() = default;
    virtual int init(json a_jmsg);
    virtual int put(const void *a_data, json &a_jmsg);
    virtual int get(void *a_data, json &a_jmsg);
    int put_streams(const void *a_data, json &a_jmsg);
    void flush();
    void add_stream(json a_jmsg);
    void add_file(std::string p_method);
    std::string name() { return "DataManager"; }
    std::string type() { return "Manager"; }
    virtual void transform(std::vector<char> &a_data, json &a_jmsg) {}

private:
    CacheMan m_cache;
    size_t m_cache_size = 0;
    size_t m_timestep = 0;
    int m_stream_index = 0;
};

#endif // DATAMAN_DATAMAN_H_
