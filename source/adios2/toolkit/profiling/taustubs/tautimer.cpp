/*
   Copyright (c) 2019 University of Oregon
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "tautimer.hpp"
/* If not enabled, macro out all of the code in this file. */
#if defined(TAU_USE_STUBS)

#include <unistd.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <thread>

/* Clean assertion handling */
inline void _tautimer_assert(const char *expression, const char *file, int line)
{
    fprintf(stderr, "Assertion '%s' failed, file '%s' line '%d'.", expression,
            file, line);
    abort();
}

#ifdef NDEBUG
#define TAUTIMER_ASSERT(EXPRESSION) ((void)0)
#else
#define TAUTIMER_ASSERT(EXPRESSION)                                            \
    ((EXPRESSION) ? (void)0 : _tautimer_assert(#EXPRESSION, __FILE__, __LINE__))
#endif

/* some global static variables to check which thread we are on */
pid_t mypid(0);
thread_local uint64_t mytid(0UL);
/* unfortunately, the C++ support just tells us
 * that the ID is "uninitialized thread"
 * on OSX when the library is loaded.  Bummer. */
std::thread::id main_thread_id = std::this_thread::get_id();

/* Make sure that the TauTimer singleton is constructed when the
 * library is loaded.  This will ensure (on linux, anyway) that
 * we can assert that we have initialized it on the main thread. */
static void __attribute__((constructor)) initialize_library(void);

/* Function pointer types */

typedef int (*Tau_init_type)(int argc, char **argv);
typedef int (*Tau_register_thread_type)(void);
typedef int (*Tau_create_top_level_timer_if_necessary_type)(void);
typedef int (*Tau_start_type)(const char *);
typedef int (*Tau_stop_type)(const char *);
typedef int (*Tau_exit_type)(const char *);
typedef int (*Tau_dump_prefix_type)(const char *prefix);
typedef int (*Tau_set_node_type)(int);
typedef int (*Tau_profile_exit_all_threads_type)(void);
typedef int (*Tau_get_thread_type)(void);
typedef int (*Tau_profile_exit_all_tasks_type)(void);
typedef int (*Tau_global_stop_type)(void);
typedef int (*Tau_trigger_context_event_type)(char *, double);
typedef int (*Tau_metadata_type)(const char *, const char *);
typedef void (*Tau_destructor_trigger_type)(void);

/* Function pointers */

int (*my_Tau_init)(int, char **) = NULL;
int (*my_Tau_register_thread)(void) = NULL;
int (*my_Tau_create_top_level_timer_if_necessary)(void) = NULL;
int (*my_Tau_start)(const char *) = NULL;
int (*my_Tau_stop)(const char *) = NULL;
int (*my_Tau_exit)(const char *) = NULL;
int (*my_Tau_set_node)(int) = NULL;
int (*my_Tau_dump_prefix)(const char *prefix) = NULL;
int (*my_Tau_profile_exit_all_threads)(void) = NULL;
int (*my_Tau_get_thread)(void) = NULL;
int (*my_Tau_profile_exit_all_tasks)(void) = NULL;
int (*my_Tau_global_stop)(void) = NULL;
int (*my_Tau_trigger_context_event)(char *, double) = NULL;
int (*my_Tau_metadata)(const char *, const char *) = NULL;
void (*my_Tau_destructor_trigger)(void) = NULL;

static void initialize_library(void)
{
    // initialize the library when it is loaded
    static taustubs::TauTimer &tt = taustubs::TauTimer::get();
}

void open_preload_libraries(void)
{
/* We might be in a static executable.  Get the ld_preload variable */
#if defined(__APPLE__) && defined(__MACH__)
    const char *envvar = "DYLD_PRELOAD";
#else
    const char *envvar = "LD_PRELOAD";
#endif
    const char *preload = getenv(envvar);
    if (preload != NULL)
    {
        // fprintf(stderr, "%s:\n%s\n", envvar, preload);
        /* tokenize it */
        char *token = strtok((char *)preload, ":");
        while (token)
        {
            printf("token: %s\n", token);
            /* then, dlopen() first and re-try the dlsym() call. */
            dlopen(token, RTLD_LAZY);
            token = strtok(NULL, ":");
        }
    }
    // fprintf(stderr, "%s not set!\n", envvar);
}

int assign_function_pointers(void)
{
    my_Tau_init = (Tau_init_type)dlsym(RTLD_DEFAULT, "Tau_init");
    if (my_Tau_init == NULL)
    {
        open_preload_libraries();
        my_Tau_init = (Tau_init_type)dlsym(RTLD_DEFAULT, "Tau_init");
        if (my_Tau_init == NULL)
        {
            /* Optional - print an error message, because TAU wasn't preloaded!
             */
            // fprintf(stderr, "TAU libraries not loaded, TAU support
            // unavailable\n%s\n", dlerror());
            return 1;
        }
    }
    my_Tau_register_thread =
        (Tau_register_thread_type)dlsym(RTLD_DEFAULT, "Tau_register_thread");
    my_Tau_create_top_level_timer_if_necessary =
        (Tau_create_top_level_timer_if_necessary_type)dlsym(
            RTLD_DEFAULT, "Tau_create_top_level_timer_if_necessary");
    my_Tau_start = (Tau_start_type)dlsym(RTLD_DEFAULT, "Tau_start");
    my_Tau_stop = (Tau_stop_type)dlsym(RTLD_DEFAULT, "Tau_stop");
    my_Tau_dump_prefix =
        (Tau_dump_prefix_type)dlsym(RTLD_DEFAULT, "Tau_dump_prefix");
    my_Tau_exit = (Tau_exit_type)dlsym(RTLD_DEFAULT, "Tau_exit");
    my_Tau_set_node = (Tau_set_node_type)dlsym(RTLD_DEFAULT, "Tau_set_node");
    my_Tau_profile_exit_all_threads = (Tau_profile_exit_all_threads_type)dlsym(
        RTLD_DEFAULT, "Tau_profile_exit_all_threads");
    my_Tau_get_thread =
        (Tau_get_thread_type)dlsym(RTLD_DEFAULT, "Tau_get_thread");
    my_Tau_profile_exit_all_tasks = (Tau_profile_exit_all_tasks_type)dlsym(
        RTLD_DEFAULT, "Tau_profile_exit_all_tasks");
    my_Tau_global_stop =
        (Tau_global_stop_type)dlsym(RTLD_DEFAULT, "Tau_global_stop");
    my_Tau_trigger_context_event = (Tau_trigger_context_event_type)dlsym(
        RTLD_DEFAULT, "Tau_trigger_context_event");
    my_Tau_metadata = (Tau_metadata_type)dlsym(RTLD_DEFAULT, "Tau_metadata");
    my_Tau_destructor_trigger = (Tau_destructor_trigger_type)dlsym(
        RTLD_DEFAULT, "Tau_destructor_trigger");
    return 0;
}

int tau_stub_initialize_simple_(void)
{
    if (assign_function_pointers() == 1)
    {
        return 1;
    }
    int _argc = 1;
    const char *_dummy = "";
    char *_argv[1];
    _argv[0] = (char *)(_dummy);
    my_Tau_init(_argc, _argv);
    // Not necessary to set the node id, the TAU MPI_Init wrapper will do that
    my_Tau_create_top_level_timer_if_necessary();
    return 0;
}

void tau_stub_exit(void)
{
    if (my_Tau_destructor_trigger != NULL)
    {
        my_Tau_destructor_trigger();
    }
    if (my_Tau_profile_exit_all_threads != NULL)
    {
        my_Tau_profile_exit_all_threads();
    }
    if (my_Tau_exit != NULL)
    {
        my_Tau_exit("stub exiting");
    }
}

namespace taustubs
{

thread_local bool TauTimer::thread_seen(false);

// constructor
TauTimer::TauTimer(void) : initialized(false)
{
    // Confirm that we are instantializing this singleton on the main thread!
    mypid = getpid();
#if defined(__APPLE__) && defined(__MACH__)
    // why should Apple support gettid? BE DIFFERENT, BABY!
    pthread_threadid_np(NULL, &mytid);
// this assertion doesn't work correctly.  Bummer.
// TAUTIMER_ASSERT(main_thread_id == std::this_thread::get_id());
#else
    mytid = (uint64_t)syscall(__NR_gettid);
    // only true on Linux.  Bummer.
    TAUTIMER_ASSERT(mytid == mypid);
#endif
    if (tau_stub_initialize_simple_() == 0)
        initialized = true;
    thread_seen = true;
}

// The only way to get an instance of this class
TauTimer &TauTimer::get(void)
{
    static std::unique_ptr<TauTimer> instance(new TauTimer);
    if (!thread_seen && instance.get()->initialized)
    {
        _RegisterThread();
    }
    return *instance;
}

// used internally to the class
inline void TauTimer::_RegisterThread(void)
{
    if (!thread_seen)
    {
// Confirm that we are NOT on the main thread!
#if defined(__APPLE__) && defined(__MACH__)
        // why should Apple support gettid? BE DIFFERENT, BABY!
        pthread_threadid_np(NULL, &mytid);
// this assertion doesn't work correctly.  Bummer.
// TAUTIMER_ASSERT(main_thread_id != std::this_thread::get_id());
#else
        mytid = (uint64_t)syscall(__NR_gettid);
        // only true on Linux.  Bummer.
        TAUTIMER_ASSERT(mytid != mypid);
#endif
        my_Tau_register_thread();
        my_Tau_create_top_level_timer_if_necessary();
        thread_seen = true;
    }
}

// external API call
void TauTimer::RegisterThread(void)
{
    if (!TauTimer::get().initialized)
        return;
    _RegisterThread();
}

void TauTimer::Start(const char *timer_name)
{
    if (!TauTimer::get().initialized)
        return;
    my_Tau_start(timer_name);
}

void TauTimer::Start(const std::string &timer_name)
{
    if (!TauTimer::get().initialized)
        return;
    my_Tau_start(timer_name.c_str());
}

void TauTimer::Stop(const char *timer_name)
{
    if (!TauTimer::get().initialized)
        return;
    my_Tau_stop(timer_name);
}

void TauTimer::Stop(const std::string &timer_name)
{
    if (!TauTimer::get().initialized)
        return;
    my_Tau_stop(timer_name.c_str());
}

void TauTimer::SampleCounter(const char *name, const double value)
{
    if (!TauTimer::get().initialized)
        return;
    my_Tau_trigger_context_event(const_cast<char *>(name), value);
}

void TauTimer::MetaData(const char *name, const char *value)
{
    if (!TauTimer::get().initialized)
        return;
    my_Tau_metadata(name, value);
}

TauTimer::~TauTimer(void) { tau_stub_exit(); }

} // namespace taustubs

/* Expose the API to C and Fortran */

extern "C" {
// C Bindings
void TauTimer_RegisterThread() { taustubs::TauTimer::RegisterThread(); }

void TauTimer_Start(const char *timer_name)
{
    taustubs::TauTimer::Start(timer_name);
}

void TauTimer_Stop(const char *timer_name)
{
    taustubs::TauTimer::Stop(timer_name);
}

void TauTimer_SampleCounter(const char *name, const double value)
{
    taustubs::TauTimer::SampleCounter(name, value);
}

void TauTimer_MetaData(const char *name, const char *value)
{
    taustubs::TauTimer::MetaData(name, value);
}

// Fortran Bindings
void tautimer_registerthread_() { taustubs::TauTimer::RegisterThread(); }

void tautimer_start_(const char *timer_name)
{
    taustubs::TauTimer::Start(timer_name);
}

void tautimer_stop_(const char *timer_name)
{
    taustubs::TauTimer::Stop(timer_name);
}

void tautimer_samplecounter_(const char *name, const double value)
{
    taustubs::TauTimer::SampleCounter(name, value);
}

void tautimer_metadata_(const char *name, const char *value)
{
    taustubs::TauTimer::MetaData(name, value);
}

} // extern "C"

#endif // defined(TAU_USE_STUBS)
