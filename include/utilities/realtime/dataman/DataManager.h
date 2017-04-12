#ifndef DATAMANAGER_H_
#define DATAMANAGER_H_

#include "utilities/realtime/dataman/DataMan.h"

namespace adios
{
namespace realtime
{

class DataManager : public DataMan
{
public:
    DataManager();
    ~DataManager();
    virtual int init(json p_jmsg);
    virtual int put(const void *p_data, json p_jmsg);
    virtual int get(void *p_data, json &p_jmsg);
    void flush();
    void add_stream(json p_jmsg);
    int put(const void *p_data, string p_doid, string p_var, string p_dtype,
            vector<size_t> p_putshape, vector<size_t> p_varshape,
            vector<size_t> p_offset, size_t p_timestep, int p_tolerance = 0,
            int p_priority = 100);
    void add_file(string p_method);
    string name() { return "DataManager"; }
    string type() { return "Manager"; }
    virtual void transform(const void *p_in, void *p_out, json &p_jmsg){};

private:
    string m_local_ip = "";
    string m_remote_ip = "";
    int m_local_port = 0;
    int m_remote_port = 0;
    int m_num_channels = 0;
    vector<int> m_tolerance;
    vector<int> m_priority;
};

// end namespace realtime
}
// end namespace adios
}
#endif
