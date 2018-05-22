#include "config.h"
#include "dill.h"
#include "dill_internal.h"
#include "sys/mman.h"
#ifdef HAVE_MEMORY_H
#include "memory.h"
#endif
#include "x86.h"

extern double dill_x86_64_hidden_ULtoD(unsigned long a)
{ return (double) a; }
extern unsigned long dill_x86_64_hidden_DtoUL(double a)
{ unsigned long l = a; return l; }

static xfer_entry x86_64_xfer_recs[5] = {
    {"dill_x86_64_hidden_ULtoD", dill_x86_64_hidden_ULtoD},
    {"dill_x86_64_hidden_DtoUL", dill_x86_64_hidden_DtoUL},
    {(char*)0, (void*)0}};

extern void
x86_64_rt_call_link(char *code, call_t *t)
{
    int i;

    for(i=0; i< t->call_count; i++) {
	unsigned long tmp = (unsigned long) t->call_locs[i].xfer_addr;
	long *call_addr = (long *) (code + t->call_locs[i].loc + 2);
	memcpy(call_addr, &tmp, 8);
    }
}

static void
x86_64_flush(void *base, void *limit)
{
#if defined(HOST_X86_64)
    {
	volatile void *ptr = base;

	/* flush every 8 bytes of preallocated insn stream. */
	while((char*)ptr < (char*) limit) {
	    asm volatile ("clflush (%0)" : /* */ : "r" (ptr));
	    ptr = (char *)ptr + 8;
	}
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
    }
#endif
}    

extern char *
x86_64_package_stitch(char *code, call_t *t, dill_pkg pkg)
{
    char *tmp = code;
    dill_lookup_xfer_addrs(t, &x86_64_xfer_recs[0]);
    x86_64_rt_call_link(code, t);
    x86_64_flush(code, code + 1024);
#ifdef USE_MMAP_CODE_SEG
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
    tmp = (void*)mmap(0, pkg->code_size,
		      PROT_EXEC | PROT_READ | PROT_WRITE, 
		      MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    memcpy(tmp, code, pkg->code_size);
#endif
    return tmp + pkg->entry_offset;
}
