#ifndef DATAMAN_H_
#define DATAMAN_H_

#include <chrono>
#include <cstdint>
#include <dlfcn.h>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

#include "external/json.hpp"

using json = nlohmann::json;
using namespace std;

namespace adios
{
namespace realtime
{

class DataMan
{
public:
    DataMan()
    {
        m_profiling["total_manager_time"] = 0.0f;
        m_profiling["total_mb"] = 0.0f;
        m_start_time = chrono::system_clock::now();
    }
    virtual ~DataMan() {}
    int put(const void *p_data, string p_doid, string p_var, string p_dtype,
            vector<size_t> p_putshape, vector<size_t> p_varshape,
            vector<size_t> p_offset, size_t p_timestep, int p_tolerance = 0,
            int p_priority = 100)
    {
        json msg;
        msg["doid"] = p_doid;
        msg["var"] = p_var;
        msg["dtype"] = p_dtype;
        msg["putshape"] = p_putshape;
        msg["putbytes"] = product(p_putshape, dsize(p_dtype));
        msg["varshape"] = p_varshape;
        msg["varbytes"] = product(p_varshape, dsize(p_dtype));
        msg["offset"] = p_offset;
        msg["timestep"] = p_timestep;
        msg["tolerance"] = p_tolerance;
        msg["priority"] = p_priority;
        return put(p_data, msg);
    }

    virtual int put_begin(const void *p_data, json &p_jmsg)
    {
        check_shape(p_jmsg);
        p_jmsg["profiling"] = m_profiling;
        m_step_time = chrono::system_clock::now();
        return 0;
    }

    virtual int put_end(const void *p_data, json &p_jmsg)
    {
        auto end = chrono::system_clock::now();
        chrono::duration<double> duration = end - m_step_time;
        m_profiling["total_manager_time"] =
            m_profiling["total_manager_time"].get<double>() + duration.count();
        m_profiling["total_mb"] =
            m_profiling["total_mb"].get<size_t>() +
            product(p_jmsg["varshape"], dsize(p_jmsg["dtype"])) / 1000000.0f;
        duration = end - m_start_time;
        m_profiling["total_workflow_time"] = duration.count();
        m_profiling["workflow_mbs"] =
            m_profiling["total_mb"].get<double>() /
            m_profiling["total_workflow_time"].get<double>();
        m_profiling["manager_mbs"] =
            m_profiling["total_mb"].get<double>() /
            m_profiling["total_manager_time"].get<double>();
        if (p_jmsg["compressed_size"] != nullptr)
            p_jmsg["putbytes"] = p_jmsg["compressed_size"].get<size_t>();
        put_next(p_data, p_jmsg);
        return 0;
    }

    virtual int put(const void *p_data, json p_jmsg) = 0;

    int get(void *p_data, string p_doid, string p_var, string p_dtype,
            vector<size_t> p_getshape, vector<size_t> p_varshape,
            vector<size_t> p_offset, size_t p_timestep)
    {
        json msg;
        msg["doid"] = p_doid;
        msg["var"] = p_var;
        msg["dtype"] = p_dtype;
        msg["getshape"] = p_getshape;
        msg["varshape"] = p_varshape;
        msg["offset"] = p_offset;
        msg["timestep"] = p_timestep;
        return get(p_data, msg);
    }

    int get(void *p_data, string p_doid, string p_var, string &p_dtype,
            vector<size_t> &p_varshape, size_t &p_timestep)
    {
        json msg;
        msg["doid"] = p_doid;
        msg["var"] = p_var;
        return get(p_data, msg);
    }

    virtual int get(void *p_data, json &p_jmsg) = 0;
    virtual int init(json p_jmsg) = 0;
    virtual void flush() = 0;
    virtual string name() = 0;
    virtual string type() = 0;
    void reg_callback(std::function<void(const void *, string, string, string,
                                         vector<size_t>)>
                          cb)
    {
        if (m_next.size() == 0)
        {
            m_callback = cb;
        }
        else
        {
            for (auto i : m_next)
            {
                i.second->reg_callback(cb);
            }
        }
    }

    void dump(const void *p_data, json p_jmsg)
    {
        vector<size_t> p_varshape = p_jmsg["varshape"].get<vector<size_t>>();
        string dtype = p_jmsg["dtype"];
        size_t length = p_jmsg["dumplength"].get<size_t>();
        size_t s = 0;
        for (size_t i = 0; i < product(p_varshape, 1); i++)
        {
            s++;
            cout << ((float *)p_data)[i] << " ";
            if (s == length)
            {
                cout << endl;
                s = 0;
            }
        }
        cout << endl;
    }

    void add_next(string p_name, shared_ptr<DataMan> p_next)
    {
        m_next[p_name] = p_next;
    }

    void remove_next(string p_name) { m_next.erase(p_name); }

    bool have_next()
    {
        if (m_next.size() == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    void print_next()
    {
        for (auto i : m_next)
        {
            cout << i.second->name() << " -> ";
            i.second->print_next();
            cout << endl;
        }
    }

    virtual void transform(const void *p_in, void *p_out, json &p_jmsg) = 0;

protected:
    bool auto_transform(const void *p_in, void *p_out, json &p_jmsg)
    {
        if (p_jmsg["compression_method"] != nullptr)
        {
            auto method = p_jmsg["compression_method"];
            auto man = get_man(method);
            if (man == nullptr)
            {
                logging("Library file for compression method " +
                        p_jmsg["compression_method"].dump() + " not found!");
                return false;
            }
            man->transform(p_in, p_out, p_jmsg);
            p_jmsg.erase("compression_method");
            p_jmsg.erase("compression_rate");
            p_jmsg.erase("compressed_size");
            return true;
        }
        else
        {
            return false;
        }
    }

    void add_man_to_path(string p_new, string p_path)
    {
        if (m_next.count(p_path) > 0)
        {
            auto man = get_man(p_new);
            man->add_next(p_path, m_next[p_path]);
            this->add_next(p_new, man);
            this->remove_next(p_path);
        }
    }

    inline void logging(string p_msg, string p_man = "")
    {
        if (p_man == "")
            p_man = name();
        cout << "[";
        cout << p_man;
        cout << "]";
        cout << " ";
        cout << p_msg;
        cout << endl;
    }

    inline bool check_json(json p_jmsg, vector<string> p_strings,
                           string p_man = "")
    {
        if (p_man == "")
            p_man = name();
        for (auto i : p_strings)
        {
            if (p_jmsg[i] == nullptr)
            {
                if (p_man != "")
                {
                    logging("JSON key " + i + " not found!", p_man);
                }
                return false;
            }
        }
        return true;
    }

    virtual int flush_next()
    {
        for (auto i : m_next)
        {
            i.second->flush();
        }
        return 0;
    }

    virtual int put_next(const void *p_data, json p_jmsg)
    {
        for (auto i : m_next)
        {
            i.second->put(p_data, p_jmsg);
        }
        return 0;
    }

    inline size_t product(size_t *shape)
    {
        size_t s = 1;
        if (shape)
        {
            for (size_t i = 1; i <= shape[0]; i++)
            {
                s *= shape[i];
            }
        }
        return s;
    }

    inline size_t product(vector<size_t> shape, size_t size = 1)
    {
        return accumulate(shape.begin(), shape.end(), size,
                          multiplies<size_t>());
    }

    inline size_t dsize(string dtype)
    {
        if (dtype == "char")
            return sizeof(char);
        if (dtype == "short")
            return sizeof(short);
        if (dtype == "int")
            return sizeof(int);
        if (dtype == "long")
            return sizeof(long);
        if (dtype == "unsigned char")
            return sizeof(unsigned char);
        if (dtype == "unsigned short")
            return sizeof(unsigned short);
        if (dtype == "unsigned int")
            return sizeof(unsigned int);
        if (dtype == "unsigned long")
            return sizeof(unsigned long);
        if (dtype == "float")
            return sizeof(float);
        if (dtype == "double")
            return sizeof(double);
        if (dtype == "long double")
            return sizeof(long double);
        if (dtype == "complex<float>")
            return sizeof(float) * 2;
        if (dtype == "complex<double>")
            return sizeof(double) * 2;

        if (dtype == "int8_t")
            return sizeof(int8_t);
        if (dtype == "uint8_t")
            return sizeof(uint8_t);
        if (dtype == "int16_t")
            return sizeof(int16_t);
        if (dtype == "uint16_t")
            return sizeof(uint16_t);
        if (dtype == "int32_t")
            return sizeof(int32_t);
        if (dtype == "uint32_t")
            return sizeof(uint32_t);
        if (dtype == "int64_t")
            return sizeof(int64_t);
        if (dtype == "uint64_t")
            return sizeof(uint64_t);
        return 0;
    }

    inline json atoj(unsigned int *array)
    {
        json j;
        if (array)
        {
            if (array[0] > 0)
            {
                j = {array[1]};
                for (unsigned int i = 2; i <= array[0]; i++)
                {
                    j.insert(j.end(), array[i]);
                }
            }
        }
        return j;
    }

    inline string rmquote(string in) { return in.substr(1, in.length() - 2); }

    inline bool isin(string a, json j)
    {
        for (unsigned int i = 0; i < j.size(); i++)
        {
            if (j[i] == a)
                return true;
        }
        return false;
    }

    inline int closest(int v, json j, bool up)
    {
        int s = 100, k = 0, t;
        for (unsigned int i = 0; i < j.size(); i++)
        {
            if (up)
                t = j[i].get<int>() - v;
            else
                t = v - j[i].get<int>();
            if (t >= 0 && t < s)
            {
                s = t;
                k = i;
            }
        }
        return k;
    }

    inline void check_shape(json &p_jmsg)
    {
        vector<size_t> varshape;
        if (check_json(p_jmsg, {"varshape"}))
        {
            varshape = p_jmsg["varshape"].get<vector<size_t>>();
        }
        else
        {
            return;
        }
        if (p_jmsg["putshape"] == nullptr)
        {
            p_jmsg["putshape"] = varshape;
        }
        if (p_jmsg["offset"] == nullptr)
        {
            p_jmsg["offset"] = vector<size_t>(varshape.size(), 0);
        }
        p_jmsg["putbytes"] = product(p_jmsg["putshape"].get<vector<size_t>>(),
                                     dsize(p_jmsg["dtype"].get<string>()));
        p_jmsg["varbytes"] =
            product(varshape, dsize(p_jmsg["dtype"].get<string>()));
    }

    inline shared_ptr<DataMan> get_man(string method)
    {
        string soname = "lib" + method + "man.so";
        void *so = NULL;
        so = dlopen(soname.c_str(), RTLD_NOW);
        if (so)
        {
            shared_ptr<DataMan> (*func)() = NULL;
            func = (shared_ptr<DataMan>(*)())dlsym(so, "getMan");
            if (func)
            {
                return func();
            }
            else
            {
                logging("getMan() not found in " + soname);
            }
        }
        else
        {
            logging("Dynamic library " + soname +
                    " not found in LD_LIBRARY_PATH");
        }
        return nullptr;
    }

    string run_cmd(string cmd)
    {
        FILE *pipe = NULL;
        char buffer[2048];
        string result;
        pipe = popen(cmd.c_str(), "r");
        if (NULL == pipe)
        {
            perror("pipe");
            return "";
        }
        while (!feof(pipe))
        {
            if (fgets(buffer, sizeof(buffer), pipe) != NULL)
                result = buffer;
        }
        pclose(pipe);
        return result;
    }

    std::function<void(const void *, string, string, string, vector<size_t>)>
        m_callback;
    map<string, shared_ptr<DataMan>> m_next;

private:
    json m_profiling;
    chrono::time_point<chrono::system_clock> m_start_time;
    chrono::time_point<chrono::system_clock> m_step_time;
    bool m_profiling_enabled = false;
};

// end namespace realtime
}
// end namespace adios
}
#endif
