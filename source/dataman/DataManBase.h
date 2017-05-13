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

    virtual int put_begin(const void *a_data, json &a_jmsg);
    virtual int put_end(const void *a_data, json &a_jmsg);

    virtual int put(const void *a_data, json &a_jmsg) = 0;
    virtual int get(void *a_data, json &a_jmsg) = 0;
    virtual int init(json a_jmsg) = 0;
    virtual void flush() = 0;
    virtual std::string name() = 0;
    virtual std::string type() = 0;
    void reg_callback(std::function<void(const void *, std::string, std::string,
                                         std::string, std::vector<size_t>)>
                          cb);
    void dump(const void *a_data, json a_jmsg, std::ostream &out = std::cout);
    virtual void transform(std::vector<char> &a_data, json &a_jmsg) = 0;
    void dump_profiling();

protected:
    bool auto_transform(std::vector<char> &a_data, json &a_jmsg);

    std::shared_ptr<DataManBase> get_man(std::string method);

    void logging(std::string p_msg, std::string p_man = "",
                 std::ostream &out = std::cout);

    bool check_json(json a_jmsg, std::vector<std::string> p_strings,
                    std::string p_man = "");

    size_t product(size_t *shape);

    size_t product(std::vector<size_t> shape, size_t size = 1);

    size_t dsize(std::string dtype);

    int closest(int v, json j, bool up);

    void check_shape(json &a_jmsg);

    std::function<void(const void *, std::string, std::string, std::string,
                       std::vector<size_t>)>
        m_callback;
    std::vector<std::shared_ptr<DataManBase>> m_stream_mans;

private:
    struct ManagerLibrary;
    std::unordered_map<std::string, ManagerLibrary *> m_LoadedManagers;

    std::chrono::time_point<std::chrono::system_clock> m_start_time;
    std::chrono::time_point<std::chrono::system_clock> m_step_time;
    json m_profiling;
};

#endif /* DATAMANBASE_H_ */
