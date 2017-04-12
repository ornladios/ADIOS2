#include "utilities/realtime/dataman/DataManager.h"

namespace adios
{
namespace realtime
{

DataManager::DataManager() : DataMan() {}
DataManager::~DataManager() {}

int DataManager::init(json p_jmsg) { return 0; }

int DataManager::put(const void *p_data, string p_doid, string p_var,
                     string p_dtype, vector<size_t> p_putshape,
                     vector<size_t> p_varshape, vector<size_t> p_offset,
                     size_t p_timestep, int p_tolerance, int p_priority)
{
    return DataMan::put(p_data, p_doid, p_var, p_dtype, p_putshape, p_varshape,
                        p_offset, p_timestep, p_tolerance, p_priority);
}

int DataManager::put(const void *p_data, json p_jmsg)
{
    put_begin(p_data, p_jmsg);
    put_end(p_data, p_jmsg);
    return 0;
}

void DataManager::add_file(string p_method) {}

void DataManager::add_stream(json p_jmsg)
{

    string method = "zmq";

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
    man->init(p_jmsg);
    this->add_next(method, man);

    add_man_to_path("zfp", method);
}

void DataManager::flush() { flush_next(); }

int DataManager::get(void *p_data, json &p_jmsg) { return 0; }

// end namespace realtime
}
// end namespace adios
}
