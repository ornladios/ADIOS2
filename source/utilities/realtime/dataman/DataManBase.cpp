/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManBase.cpp
 *
 *  Created on: Apr 12, 2017
 *      Author: Jason Wang
 */

#include "utilities/realtime/dataman/DataManBase.h"

namespace adios
{
namespace realtime
{

DataManBase::DataManBase()
{
    m_profiling["total_manager_time"] = 0.0f;
    m_profiling["total_mb"] = 0.0f;
    m_start_time = std::chrono::system_clock::now();
}

int DataManBase::put(const void *p_data, std::string p_doid, std::string p_var,
                     std::string p_dtype, std::vector<size_t> p_putshape,
                     std::vector<size_t> p_varshape,
                     std::vector<size_t> p_offset, size_t p_timestep,
                     int p_tolerance, int p_priority)
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

int DataManBase::put_begin(const void *p_data, json &p_jmsg)
{
    check_shape(p_jmsg);
    p_jmsg["profiling"] = m_profiling;
    m_step_time = std::chrono::system_clock::now();
    return 0;
}

int DataManBase::put_end(const void *p_data, json &p_jmsg)
{
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> duration = end - m_step_time;
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

int DataManBase::get(void *p_data, std::string p_doid, std::string p_var,
                     std::string p_dtype, std::vector<size_t> p_getshape,
                     std::vector<size_t> p_varshape,
                     std::vector<size_t> p_offset, size_t p_timestep)
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

int DataManBase::get(void *p_data, std::string p_doid, std::string p_var,
                     std::string &p_dtype, std::vector<size_t> &p_varshape,
                     size_t &p_timestep)
{
    json msg;
    msg["doid"] = p_doid;
    msg["var"] = p_var;
    return get(p_data, msg);
}

void DataManBase::reg_callback(
    std::function<void(const void *, std::string, std::string, std::string,
                       std::vector<size_t>)>
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

void DataManBase::dump(const void *p_data, json p_jmsg, std::ostream &out)
{
    std::vector<size_t> p_varshape =
        p_jmsg["varshape"].get<std::vector<size_t>>();
    std::string dtype = p_jmsg["dtype"];
    size_t length = p_jmsg["dumplength"].get<size_t>();
    size_t s = 0;
    for (size_t i = 0; i < product(p_varshape, 1); i++)
    {
        s++;
        out << ((float *)p_data)[i] << " ";
        if (s == length)
        {
            out << std::endl;
            s = 0;
        }
    }
    out << std::endl;
}

void DataManBase::add_next(std::string p_name,
                           std::shared_ptr<DataManBase> p_next)
{
    m_next[p_name] = p_next;
}

void DataManBase::remove_next(std::string p_name) { m_next.erase(p_name); }

bool DataManBase::have_next()
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

void DataManBase::print_next(std::ostream &out)
{
    for (auto i : m_next)
    {
        out << i.second->name() << " -> ";
        i.second->print_next();
        out << std::endl;
    }
}

bool DataManBase::auto_transform(const void *p_in, void *p_out, json &p_jmsg)
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

void DataManBase::add_man_to_path(std::string p_new, std::string p_path)
{
    if (m_next.count(p_path) > 0)
    {
        auto man = get_man(p_new);
        if (man)
        {
            man->add_next(p_path, m_next[p_path]);
            this->add_next(p_new, man);
            this->remove_next(p_path);
        }
    }
}

int DataManBase::flush_next()
{
    for (auto i : m_next)
    {
        i.second->flush();
    }
    return 0;
}

int DataManBase::put_next(const void *p_data, json p_jmsg)
{
    for (auto i : m_next)
    {
        i.second->put(p_data, p_jmsg);
    }
    return 0;
}

std::shared_ptr<DataManBase> DataManBase::get_man(std::string method)
{
    void *so = NULL;
#ifdef __APPLE__
    std::string dylibname = "lib" + method + "man.dylib";
    so = dlopen(dylibname.c_str(), RTLD_NOW);
    if (so)
    {
        std::shared_ptr<DataManBase> (*func)() = NULL;
        func = (std::shared_ptr<DataManBase>(*)())dlsym(so, "getMan");
        if (func)
        {
            return func();
        }
    }
#endif
    std::string soname = "lib" + method + "man.so";
    so = dlopen(soname.c_str(), RTLD_NOW);
    if (so)
    {
        std::shared_ptr<DataManBase> (*func)() = NULL;
        func = (std::shared_ptr<DataManBase>(*)())dlsym(so, "getMan");
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
        logging("Dynamic library " + soname + " not found in LD_LIBRARY_PATH");
    }
    return nullptr;
}

// end namespace realtime
}
// end namespace adios
}
