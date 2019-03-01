/*
   Copyright (c) 2019 University of Oregon
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

/* This code won't compile on windows.  Disable it */
#if !defined(_WIN32) && !defined(_WIN64)
#define TAU_USE_STUBS
#endif

#if defined(TAU_USE_STUBS)

#include <memory>
#include <sstream>
#include <string>

namespace taustubs
{

class TauTimer
{
public:
    static void RegisterThread(void);
    static void _RegisterThread(void);
    static void Start(const char *timer_name);
    static void Start(const std::string &timer_name);
    static void Stop(const char *timer_name);
    static void Stop(const std::string &timer_name);
    static void SampleCounter(const char *name, const double value);
    static void MetaData(const char *name, const char *value);
    // The only way to get an instance of this class
    static TauTimer &get(void);
    // destructor
    ~TauTimer(void);
    bool initialized;
    static thread_local bool thread_seen;

private:
    // Private constructor to prevent construction
    TauTimer(void);
    // Prevent copies
    TauTimer(const TauTimer &old);
    const TauTimer &operator=(const TauTimer &old);
};

class scoped_timer
{
private:
    std::string _name;

public:
    scoped_timer(const std::string &name) : _name(name)
    {
        TauTimer::Start(_name);
    }
    ~scoped_timer() { TauTimer::Stop(_name); }
};

} // namespace taustubs

#define TAU_REGISTER_THREAD() taustubs::TauTimer::RegisterThread();
#define TAU_START(_timer_name) taustubs::TauTimer::Start(_timer_name);
#define TAU_STOP(_timer_name) taustubs::TauTimer::Stop(_timer_name);
#define TAU_SAMPLE_COUNTER(_name, _value)                                      \
    taustubs::TauTimer::SampleCounter(_name, _value);
#define TAU_METADATA(_name, _value) taustubs::TauTimer::MetaData(_name, _value);
#define TAU_SCOPED_TIMER(__name) taustubs::scoped_timer __var##finfo(__name);
#define TAU_SCOPED_TIMER_FUNC()                                                \
    std::stringstream __ss##finfo;                                             \
    __ss##finfo << __func__ << " [{" << __FILE__ << "} {" << __LINE__          \
                << ",0}]";                                                     \
    taustubs::scoped_timer __var##finfo(__ss##finfo.str());

#else // defined(TAU_USE_STUBS)

#define TAU_REGISTER_THREAD()
#define TAU_START(_timer_name)
#define TAU_STOP(_timer_name)
#define TAU_START_FUNC()
#define TAU_STOP_FUNC()
#define TAU_SAMPLE_COUNTER(_name, _value)
#define TAU_METADATA(_name, _value)
#define TAU_SCOPED_TIMER(__name)
#define TAU_SCOPED_TIMER_FUNC()

#endif // defined(TAU_USE_STUBS)
