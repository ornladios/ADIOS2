/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CacheMan.cpp
 *
 *  Created on: Apr 18, 2017
 *      Author: Jason Wang
 */

#include "CacheMan.h"

#include <algorithm>
#include <limits>

int CacheItem::init(json a_jmsg)
{
    m_jmsg = a_jmsg;
    return 0;
}

int CacheMan::put(const void *a_data, json a_jmsg)
{
    std::string doid = a_jmsg["doid"].get<std::string>();
    std::string var = a_jmsg["var"].get<std::string>();
    return m_cache[doid][var].put(a_data, a_jmsg);
}

int CacheItem::put(const void *a_data, json a_jmsg)
{
    if (!m_initialized)
    {
        init(a_jmsg);
        m_initialized = true;
    }

    std::vector<size_t> varshape =
        a_jmsg["varshape"].get<std::vector<size_t>>();
    std::vector<size_t> putshape =
        a_jmsg["putshape"].get<std::vector<size_t>>();
    std::vector<size_t> offset = a_jmsg["offset"].get<std::vector<size_t>>();
    size_t putsize = a_jmsg["putsize"].get<size_t>();
    size_t chunksize = putshape.back();
    size_t varbytes = a_jmsg["varbytes"].get<size_t>();
    size_t dsize = a_jmsg["dsize"].get<size_t>();

    if (m_cache.empty())
    {
        push();
    }

    printf("put begin front, pointer=%d\n", m_cache.front().data());
    printf("put begin back, pointer=%d\n", m_cache.back().data());

    for (size_t i = 0; i < putsize; i += chunksize)
    {
        std::vector<size_t> p = one2multi(putshape, i);
        p = apply_offset(p, offset);
        size_t ig = multi2one(varshape, p);
        std::copy((char *)a_data + i * dsize,
                  (char *)a_data + i * dsize + chunksize * dsize,
                  m_cache.back().data() + ig * dsize);
    }

    printf("put end front, pointer=%d\n", m_cache.front().data());
    printf("put end back, pointer=%d\n", m_cache.back().data());
    return 0;
}

void *CacheItem::get() {

    printf("get, pointer=%d\n", m_cache.front().data());
    return m_cache.front().data();
}

void *CacheMan::get(std::string doid, std::string var)
{
    return m_cache[doid][var].get();
}

void CacheMan::pop()
{
    for (auto i : m_cache)
    {
        for (auto j : i.second)
        {
            j.second.pop();
        }
    }
    m_timesteps_cached--;
    m_timestep_first++;
}

void CacheItem::pop()
{
    std::cout << "CacheItem::pop() ---------------------------" << std::endl;
    m_cache.pop();
}

void CacheMan::push()
{
    for (auto i : m_cache)
    {
        for (auto j : i.second)
        {
            j.second.push();
        }
    }
    m_timesteps_cached++;
}

void CacheItem::push()
{
    std::cout << "CacheItem::push() ---------------------------" << std::endl;
    size_t varbytes = m_jmsg["varbytes"].get<size_t>();
    std::vector<char> buff(varbytes);
    m_cache.push(buff);
    clean(m_cache.front(), "nan");
}

void CacheItem::clean(std::string a_mode)
{
    size_t varbytes = m_jmsg["varbytes"].get<size_t>();
    size_t varsize = m_jmsg["varsize"].get<size_t>();
    std::string dtype = m_jmsg["dtype"].get<std::string>();
    if (a_mode == "zero")
    {
        printf("zero, pointer=%d\n", m_cache.front().data());
        std::memset(m_cache.front().data(), 0, varbytes);
        return;
    }
    else if (a_mode == "nan")
    {
        if (dtype == "float")
        {
            printf("nan, pointer=%d\n", m_cache.front().data());
            for (size_t j = 0; j < varsize; j++)
            {
                ((float*)(m_cache.front().data()))[j] =
                    std::numeric_limits<float>::quiet_NaN();
            }
        }
        else if (dtype == "double")
        {
            for (size_t j = 0; j < varsize; j++)
            {
                ((double *)m_cache.front().data())[j] =
                    std::numeric_limits<double>::quiet_NaN();
            }
        }
        else if (dtype == "int")
        {
            for (size_t j = 0; j < varsize; j++)
            {
                ((int *)m_cache.front().data())[j] =
                    std::numeric_limits<int>::quiet_NaN();
            }
        }
    }
//    clean(m_cache.front(), a_mode);
}

void CacheMan::clean(std::string a_mode)
{
    for (auto i : m_cache)
    {
        for (auto j : i.second)
        {
            j.second.clean(a_mode);
        }
    }
}

void CacheItem::clean(std::vector<char> &a_data, std::string a_mode)
{
    size_t varbytes = m_jmsg["varbytes"].get<size_t>();
    size_t varsize = m_jmsg["varsize"].get<size_t>();
    std::string dtype = m_jmsg["dtype"].get<std::string>();
    if (a_mode == "zero")
    {
        printf("zero, pointer=%d\n", a_data.data());
        std::memset(a_data.data(), 0, varbytes);
        return;
    }
    else if (a_mode == "nan")
    {
        if (dtype == "float")
        {
            printf("nan, pointer=%d\n", a_data.data());
            for (size_t j = 0; j < varsize; j++)
            {
                ((float*)(a_data.data()))[j] =
                    std::numeric_limits<float>::quiet_NaN();
            }
        }
        else if (dtype == "double")
        {
            for (size_t j = 0; j < varsize; j++)
            {
                ((double *)a_data.data())[j] =
                    std::numeric_limits<double>::quiet_NaN();
            }
        }
        else if (dtype == "int")
        {
            for (size_t j = 0; j < varsize; j++)
            {
                ((int *)a_data.data())[j] =
                    std::numeric_limits<int>::quiet_NaN();
            }
        }
    }
}

std::vector<std::string> CacheMan::get_do_list()
{
    std::vector<std::string> do_list;
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it)
        do_list.push_back(it->first);
    return do_list;
}

std::vector<std::string> CacheMan::get_var_list(std::string doid)
{
    std::vector<std::string> var_list;
    for (auto it = m_cache[doid].begin(); it != m_cache[doid].end(); ++it)
        var_list.push_back(it->first);
    return var_list;
}

size_t CacheMan::get_timesteps_cached() { return m_timesteps_cached; }

nlohmann::json CacheMan::get_jmsg(std::string a_doid, std::string a_var)
{
    return m_cache[a_doid][a_var].get_jmsg();
}

nlohmann::json CacheItem::get_jmsg()
{
    m_jmsg.erase("putsize");
    m_jmsg.erase("putshape");
    m_jmsg.erase("putbytes");
    m_jmsg.erase("offset");
    return m_jmsg;
}


