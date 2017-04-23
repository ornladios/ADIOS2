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

#include "DataMan.h"

class CacheItem : public DataManBase
{
public:
    CacheItem() = default;
    virtual ~CacheItem() = default;

    void init(std::string doid, std::string var, std::string dtype,
              std::vector<size_t> varshape);

    virtual int init(json p_jmsg);
    virtual int put(const void *p_data, json p_jmsg);
    virtual int get(void *p_data, json &p_jmsg);
    virtual void transform(std::vector<char> &a_data, json &a_jmsg) {}

    void flush();
    std::string name() { return "CacheItem"; }
    std::string type() { return "Cache"; }
    const void *get_buffer();
    void clean(const std::string mode);
    void remove(size_t timestep);
    std::vector<size_t> get_shape();
    std::string get_dtype();

private:
    std::map<size_t, std::vector<char>> m_buffer;
    std::string m_doid;
    std::string m_var;
    std::string m_dtype;
    size_t m_bytes;
    size_t m_varsize;
    size_t m_varbytes;
    std::vector<size_t> m_varshape;
    bool m_completed;
    size_t m_timestep = 0;

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

class CacheMan : public DataManBase
{

public:
    CacheMan() = default;
    virtual ~CacheMan() = default;

    virtual int init(json p_jmsg);
    virtual int put(const void *p_data, json p_jmsg);
    virtual int get(void *p_data, json &p_jmsg);
    virtual void transform(std::vector<char> &a_data, json &a_jmsg) {}

    void flush();
    std::string name() { return "CacheMan"; }
    std::string type() { return "Cache"; }
    const void *get_buffer(std::string doid, std::string var);
    void clean(std::string doid, std::string var, std::string mode);
    void clean_all(std::string mode);
    void remove(std::string doid, std::string var, size_t timestep);
    void remove_all(size_t timestep);
    std::vector<std::string> get_do_list();
    std::vector<std::string> get_var_list(std::string doid);
    std::vector<size_t> get_shape(std::string doid, std::string var);
    std::string get_dtype(std::string doid, std::string var);

private:
    typedef std::map<std::string, CacheItem> CacheVarMap;
    typedef std::map<std::string, CacheVarMap> CacheDoMap;
    CacheDoMap m_cache;
};

extern "C" DataManBase *getMan();

#endif
