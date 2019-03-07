/*
    Copyright (c) 2019 University of Oregon
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef TAUSTUBS_H
#define TAUSTUBS_H

/* This code won't compile on windows.  Disable it */
#if !defined(_WIN32) && !defined(_WIN64)
#define TAU_USE_STUBS
#endif

#if defined(TAU_USE_STUBS)

/* regular C API */

void TauTimer_RegisterThread();
void TauTimer_Start(const char *timer_name);
void TauTimer_Stop(const char *timer_name);
void TauTimer_SampleCounter(const char *name, const double value);
void TauTimer_MetaData(const char *name, const char *value);

/*
    Macro API for option of entirely disabling at compile time
    To use this API, set the Macro TAU_USE_STUBS on the command
    line or in a config.h file, however your project does it
 */

#define TAU_REGISTER_THREAD() TauTimer_RegisterThread();
#define TAU_START(_timer_name) TauTimer_Start(_timer_name);
#define TAU_STOP(_timer_name) TauTimer_Stop(_timer_name);
#define TAU_START_FUNC()                                                       \
    char __tauFuncName[1024];                                                  \
    sprintf(__tauFuncName, "%s [{%s} {%d,0}]", __func__, __FILE__, __LINE__);  \
    TauTimer_Start(__tauFuncName);
#define TAU_STOP_FUNC() TauTimer_Stop(__tauFuncName);
#define TAU_SAMPLE_COUNTER(_name, _value) TauTimer_SampleCounter(_name, _value);
#define TAU_METADATA(_name, _value) TauTimer_MetaData(_name, _value);

#else // defined(TAU_USE_STUBS)

#define TAU_REGISTER_THREAD()
#define TAU_START(_timer_name)
#define TAU_STOP(_timer_name)
#define TAU_START_FUNC()
#define TAU_STOP_FUNC()
#define TAU_SAMPLE_COUNTER(_name, _value)
#define TAU_METADATA(_name, _value)

#endif // defined(TAU_USE_STUBS)

#endif // TAUSTUBS_H
