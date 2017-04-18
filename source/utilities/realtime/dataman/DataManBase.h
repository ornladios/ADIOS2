/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManBase.h
 *
 *  Created on: Apr 12, 2017
 *      Author: Jason Wang
 */

#ifndef DATAMAN_H_
#define DATAMAN_H_

#include <cstdint>

#include <dlfcn.h>
#include <unistd.h>

#include <chrono>
#include <complex>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "external/json.hpp"

namespace adios
{
namespace realtime
{

class DataManBase
{
public:
    using json = nlohmann::json;
    DataManBase();
    virtual ~DataManBase() = default;
    int put(const void *p_data, std::string p_doid, std::string p_var,
            std::string p_dtype, std::vector<size_t> p_putshape,
            std::vector<size_t> p_varshape, std::vector<size_t> p_offset,
            size_t p_timestep, int p_tolerance = 0, int p_priority = 100);

    virtual int put_begin(const void *p_data, json &p_jmsg);

    virtual int put_end(const void *p_data, json &p_jmsg);

    virtual int put(const void *p_data, json p_jmsg) = 0;

    int get(void *p_data, std::string p_doid, std::string p_var,
            std::string p_dtype, std::vector<size_t> p_getshape,
            std::vector<size_t> p_varshape, std::vector<size_t> p_offset,
            size_t p_timestep);

    int get(void *p_data, std::string p_doid, std::string p_var,
            std::string &p_dtype, std::vector<size_t> &p_varshape,
            size_t &p_timestep);

    virtual int get(void *p_data, json &p_jmsg) = 0;
    virtual int init(json p_jmsg) = 0;
    virtual void flush() = 0;
    virtual std::string name() = 0;
    virtual std::string type() = 0;
    void reg_callback(std::function<void(const void *, std::string, std::string,
                                         std::string, std::vector<size_t>)>
                          cb);

    void dump(const void *p_data, json p_jmsg, std::ostream &out = std::cout);

    void add_next(std::string p_name, std::shared_ptr<DataManBase> p_next);

    void remove_next(std::string p_name);

    bool have_next();

    void print_next(std::ostream &out = std::cout);

    virtual void transform(const void *p_in, void *p_out, json &p_jmsg) = 0;

protected:
    bool auto_transform(const void *p_in, void *p_out, json &p_jmsg);

    void add_man_to_path(std::string p_new, std::string p_path);

    virtual int flush_next();

    virtual int put_next(const void *p_data, json p_jmsg);

    std::shared_ptr<DataManBase> get_man(std::string method);

    inline void logging(std::string p_msg, std::string p_man = "",
                        std::ostream &out = std::cout)
    {
        if (p_man == "")
            p_man = name();
        out << "[";
        out << p_man;
        out << "]";
        out << " ";
        out << p_msg;
        out << std::endl;
    }

    inline bool check_json(json p_jmsg, std::vector<std::string> p_strings,
                           std::string p_man = "")
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

    inline size_t product(std::vector<size_t> shape, size_t size = 1)
    {
        return accumulate(shape.begin(), shape.end(), size,
                          std::multiplies<size_t>());
    }

    inline size_t dsize(std::string dtype)
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
        if (dtype == "std::complex<float>" or dtype == "complex<float>")
            return sizeof(std::complex<float>);
        if (dtype == "std::complex<double>")
            return sizeof(std::complex<double>);

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

    inline std::string rmquote(std::string in)
    {
        return in.substr(1, in.length() - 2);
    }

    inline bool isin(std::string a, json j)
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
        std::vector<size_t> varshape;
        if (check_json(p_jmsg, {"varshape"}))
        {
            varshape = p_jmsg["varshape"].get<std::vector<size_t>>();
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
            p_jmsg["offset"] = std::vector<size_t>(varshape.size(), 0);
        }
        p_jmsg["putbytes"] =
            product(p_jmsg["putshape"].get<std::vector<size_t>>(),
                    dsize(p_jmsg["dtype"].get<std::string>()));
        p_jmsg["varbytes"] =
            product(varshape, dsize(p_jmsg["dtype"].get<std::string>()));
    }

    std::function<void(const void *, std::string, std::string, std::string,
                       std::vector<size_t>)>
        m_callback;
    std::map<std::string, std::shared_ptr<DataManBase>> m_next;

private:
    json m_profiling;
    std::chrono::time_point<std::chrono::system_clock> m_start_time;
    std::chrono::time_point<std::chrono::system_clock> m_step_time;
    bool m_profiling_enabled = false;
};

// end namespace realtime
}
// end namespace adios
}
#endif
