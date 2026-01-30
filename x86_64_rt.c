#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "dill.h"
#ifdef HAVE_SYS_MMAN_H
#include "sys/mman.h"
#endif
#ifdef HAVE_MEMORY_H
#include "memory.h"
#endif
#ifdef USE_VIRTUAL_PROTECT
#include <windows.h>
#include <intrin.h>
#include <memoryapi.h>
#endif
#ifdef USE_MACOS_MAP_JIT
#include <pthread.h>
#endif
#include "dill_internal.h"
#include "x86.h"

extern double
dill_x86_64_hidden_ULtoD(size_t a)
{
    return (double)a;
}
extern size_t
dill_x86_64_hidden_DtoUL(double a)
{
    /* Cast directly to size_t, not via long (long is 32-bit on Windows) */
    return (size_t)a;
}

static xfer_entry x86_64_xfer_recs[5] = {
    {"dill_x86_64_hidden_ULtoD", dill_x86_64_hidden_ULtoD},
    {"dill_x86_64_hidden_DtoUL", dill_x86_64_hidden_DtoUL},
    {(char*)0, (void*)0}};

extern void
x86_64_rt_call_link(char* code, call_t* t)
{
    int i;

    for (i = 0; i < t->call_count; i++) {
        uintptr_t tmp = (uintptr_t)t->call_locs[i].xfer_addr;
        long* call_addr = (long*)(code + t->call_locs[i].loc + 2);
        memcpy(call_addr, &tmp, 8);
    }
}

static void
x86_64_flush(void* base, void* limit)
{
#if defined(USE_VIRTUAL_PROTECT)
    FlushInstructionCache(GetCurrentProcess(), base, (char*)limit - (char*)base);
#elif defined(HOST_X86_64)
    {
        volatile void* ptr = base;

        /* flush every 8 bytes of preallocated insn stream. */
        while ((char*)ptr < (char*)limit) {
#ifndef _MSC_VER
#ifdef __x86_64__
            asm volatile("clflush (%0)" : /* */ : "r"(ptr));
#endif
#else
            _mm_clflush((const void*)ptr);
#endif
            ptr = (char*)ptr + 8;
        }
#ifndef _MSC_VER
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
#endif
    }
#endif
}

extern char*
x86_64_package_stitch(char* code, call_t* t, dill_pkg pkg)
{
    char* tmp = code;
    dill_lookup_xfer_addrs(t, &x86_64_xfer_recs[0]);
    x86_64_rt_call_link(code, t);
#ifdef USE_MACOS_MAP_JIT
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
    pthread_jit_write_protect_np(0);
    tmp = (void*)mmap(0, pkg->code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_ANONYMOUS | MAP_PRIVATE | MAP_JIT, -1, 0);
    if (tmp == MAP_FAILED) {
        perror("mmap MAP_JIT package_stitch");
        return NULL;
    }
    memcpy(tmp, code, pkg->code_size);
    x86_64_flush(tmp, tmp + pkg->code_size);
    pthread_jit_write_protect_np(1);
#elif defined(USE_MMAP_CODE_SEG)
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
    tmp = (void*)mmap(0, pkg->code_size, PROT_EXEC | PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    memcpy(tmp, code, pkg->code_size);
    x86_64_flush(tmp, tmp + pkg->code_size);
#elif defined(USE_VIRTUAL_PROTECT)
    {
        DWORD dummy;
        if (!VirtualProtect(tmp, pkg->code_size, PAGE_EXECUTE_READWRITE, &dummy)) {
            fprintf(stderr, "VirtualProtect failed with error %lu for address %p, size %d\n",
                    GetLastError(), (void*)tmp, pkg->code_size);
            return NULL;
        }
        x86_64_flush(tmp, tmp + pkg->code_size);
    }
#else
    x86_64_flush(code, code + pkg->code_size);
#endif
    return tmp + pkg->entry_offset;
}
