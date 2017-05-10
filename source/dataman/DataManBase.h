/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManBase.h
 *
 *  Created on: Apr 12, 2017
 *      Author: Jason Wang
 */

#ifndef DATAMAN_DATAMANBASE_H_
#define DATAMAN_DATAMANBASE_H_

#include <cstdint>

#include <chrono>
#include <complex>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <json.hpp>

class DataManBase
{
public:
    using json = nlohmann::json;
    DataManBase();

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

    virtual void transform(std::vector<char> &a_data, json &a_jmsg) = 0;

protected:
    bool auto_transform(std::vector<char> &a_data, json &a_jmsg);

    void add_man_to_path(std::string p_new, std::string p_path, json p_jmsg);

    virtual int flush_next();

    virtual int put_next(const void *p_data, json p_jmsg);

    std::shared_ptr<DataManBase> get_man(std::string method);

    void logging(std::string p_msg, std::string p_man = "",
                 std::ostream &out = std::cout);

    bool check_json(json p_jmsg, std::vector<std::string> p_strings,
                    std::string p_man = "");

    size_t product(size_t *shape);

    size_t product(std::vector<size_t> shape, size_t size = 1);

    size_t dsize(std::string dtype);

    json atoj(unsigned int *array);

    int closest(int v, json j, bool up);

    void check_shape(json &p_jmsg);

    std::function<void(const void *, std::string, std::string, std::string,
                       std::vector<size_t>)>
        m_callback;
    std::map<std::string, std::shared_ptr<DataManBase>> m_next;

private:
    struct ManagerLibrary;
    std::unordered_map<std::string, ManagerLibrary *> m_LoadedManagers;

    json m_profiling;
    std::chrono::time_point<std::chrono::system_clock> m_start_time;
    std::chrono::time_point<std::chrono::system_clock> m_step_time;
    bool m_profiling_enabled = false;
};

#endif /* DATAMANBASE_H_ */
