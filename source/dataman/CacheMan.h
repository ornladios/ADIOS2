/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CacheMan.h
 *
 *  Created on: Apr 18, 2017
 *      Author: Jason Wang
 */

#ifndef DATAMAN_CACHEMAN_H_
#define DATAMAN_CACHEMAN_H_

#include <queue>

#include "json.hpp"

class CacheItem
{
public:
    using json = nlohmann::json;

    int init(json a_jmsg);
    virtual int put(const void *a_data, json a_jmsg);
    virtual void transform(std::vector<char> &a_data, json &a_jmsg) {}

    void *get();
    void clean(const std::string a_mode);
    void pop();
    void push();
    json get_jmsg();
    void clean(std::vector<char> &a_data, std::string a_mode);
    std::string m_clean_mode;

private:
    std::queue<std::vector<char>> m_cache;
    json m_jmsg;
    bool m_initialized = false;

    inline std::vector<size_t> apply_offset(const std::vector<size_t> &p,
                                            const std::vector<size_t> &o)
    {
        std::vector<size_t> g;
        for (int i = 0; i < p.size(); i++)
        {
            g.push_back(p[i] + o[i]);
        }
        return g;
    }

    inline size_t multi2one(const std::vector<size_t> &v,
                            const std::vector<size_t> &p)
    {
        size_t index = 0;
        for (int i = 1; i < v.size(); i++)
        {
            index += std::accumulate(v.begin() + i, v.end(), p[i - 1],
                                     std::multiplies<size_t>());
        }
        index += p.back();
        return index;
    }

    inline std::vector<size_t> one2multi(const std::vector<size_t> &v, size_t p)
    {
        std::vector<size_t> index(v.size());
        for (int i = 1; i < v.size(); i++)
        {
            size_t s = std::accumulate(v.begin() + i, v.end(), 1,
                                       std::multiplies<size_t>());
            index[i - 1] = p / s;
            p -= index[i - 1] * s;
        }
        index.back() = p;
        return index;
    }
};

class CacheMan
{

public:
    using json = nlohmann::json;
    int put(const void *a_data, json a_jmsg);
    void *get(std::string doid, std::string var);
    void pop();
    void push();
    std::vector<std::string> get_do_list();
    std::vector<std::string> get_var_list(std::string doid);
    size_t get_timesteps_cached();
    json get_jmsg(std::string doid, std::string var);
    void clean(std::string a_mode);

private:
    typedef std::map<std::string, CacheItem> CacheVarMap;
    typedef std::map<std::string, CacheVarMap> CacheDoMap;
    CacheDoMap m_cache;
    size_t m_timesteps_cached = 0;
    size_t m_timestep_first = 0;
};

#endif
