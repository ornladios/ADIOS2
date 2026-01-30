/*
 * AArch64 (ARM64) runtime support for DILL
 */

#include "config.h"
#include <string.h>
#include <stdio.h>
#include "dill.h"
#include "dill_internal.h"
#include "sys/mman.h"
#ifdef HAVE_MEMORY_H
#include "memory.h"
#endif
#ifdef USE_MACOS_MAP_JIT
#include <pthread.h>
#include <libkern/OSCacheControl.h>
#endif
#include "arm64.h"

/* Hidden helper functions for operations that can't be done inline */
extern int arm64_hidden_modi(int a, int b)
{ return a % b; }
extern long arm64_hidden_mod(long a, long b)
{ return a % b; }
extern unsigned long arm64_hidden_umod(unsigned long a, unsigned long b)
{ return a % b; }
extern unsigned int arm64_hidden_umodi(unsigned int a, unsigned int b)
{ return a % b; }
extern double arm64_hidden_ultod(unsigned long a)
{ return (double)a; }
extern float arm64_hidden_ultof(unsigned long a)
{ return (float)a; }
extern unsigned long arm64_hidden_dtoul(double a)
{ return (unsigned long)a; }
extern unsigned int arm64_hidden_dtou(double a)
{ return (unsigned int)a; }
extern unsigned long arm64_hidden_ftoul(float a)
{ return (unsigned long)a; }
extern unsigned int arm64_hidden_ftou(float a)
{ return (unsigned int)a; }

static xfer_entry arm64_xfer_recs[] = {
    {"arm64_hidden_modi", arm64_hidden_modi},
    {"arm64_hidden_mod", arm64_hidden_mod},
    {"arm64_hidden_umod", arm64_hidden_umod},
    {"arm64_hidden_umodi", arm64_hidden_umodi},
    {"arm64_hidden_ultod", arm64_hidden_ultod},
    {"arm64_hidden_ultof", arm64_hidden_ultof},
    {"arm64_hidden_dtoul", arm64_hidden_dtoul},
    {"arm64_hidden_dtou", arm64_hidden_dtou},
    {"arm64_hidden_ftoul", arm64_hidden_ftoul},
    {"arm64_hidden_ftou", arm64_hidden_ftou},
    {(char*)0, (void*)0}
};

extern void
arm64_rt_call_link(char *code, call_t *t)
{
    int i;
    for (i = 0; i < t->call_count; i++) {
        /* TODO: Implement call linking for ARM64 */
        /* This would patch call sites with actual addresses */
    }
}

static void
arm64_flush(void *base, void *limit)
{
#ifdef HOST_ARM64
#ifdef USE_MACOS_MAP_JIT
    sys_icache_invalidate(base, (size_t)((char*)limit - (char*)base));
#else
    /* For Linux aarch64 */
    __builtin___clear_cache(base, limit);
#endif
#endif
}

extern char *
arm64_package_stitch(char *code, call_t *t, dill_pkg pkg)
{
    char *tmp = code;
    dill_lookup_xfer_addrs(t, &arm64_xfer_recs[0]);
    arm64_rt_call_link(code, t);

#ifdef USE_MACOS_MAP_JIT
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
    pthread_jit_write_protect_np(0);
    tmp = (void*)mmap(0, pkg->code_size,
                      PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_ANONYMOUS | MAP_PRIVATE | MAP_JIT, -1, 0);
    if (tmp == MAP_FAILED) {
        perror("mmap MAP_JIT arm64 package_stitch");
        return NULL;
    }
    memcpy(tmp, code, pkg->code_size);
    arm64_flush(tmp, tmp + pkg->code_size);
    pthread_jit_write_protect_np(1);
#elif defined(USE_MMAP_CODE_SEG)
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
    tmp = (void*)mmap(0, pkg->code_size,
                      PROT_EXEC | PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    memcpy(tmp, code, pkg->code_size);
    arm64_flush(tmp, tmp + pkg->code_size);
#else
    arm64_flush(code, code + pkg->code_size);
#endif

    return tmp + pkg->entry_offset;
}
