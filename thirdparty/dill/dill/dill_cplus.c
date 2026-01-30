#include <stdio.h>
#include "config.h"

typedef struct mp {
    union {
        void* call_ptr;
        long vtindex;
    } u;
    int delta; /* offset from base, used with multiple inheritance */
    int vflag; /* used on archs where call_ptr might be legitmately odd */
} * method_pointer;

extern void*
get_this_ptr(method_pointer m, void* object_ptr)
{
    void* th;
#if defined(HOST_ARM64)
    /* On ARM64, delta's low bit indicates virtual, so mask it off for adjustment */
    th = ((char*)object_ptr) + (m->delta >> 1);
#else
    th = ((char*)object_ptr) + (m->delta / 2);
#endif
    return th;
}

extern void*
get_xfer_ptr(method_pointer m, void* object_ptr)
{
    void* cptr;
    cptr = m->u.call_ptr;
#if defined(NOTDEF)
    if (m->vflag != 0) { /* vflag indicates virtual on some archs*/
#elif defined(HOST_ARM6) || defined(HOST_ARM7) || defined(HOST_ARM64)
    if ((m->delta & 0x1) == 1) { /* odd delta indicates virtual on ARM */
#else
    if ((m->u.vtindex & 0x1) == 1) { /* METHOD_PTR IS ODD - virtual */
#endif
#if defined(HOST_ARM64)
        /* On ARM64, vtindex is a byte offset into vtable (no adjustment needed) */
        long vtableindex = m->u.vtindex;
#else
        int vtableindex = m->u.vtindex & -4;
#endif
        void** vtable = *((void***)object_ptr);

        vtable = (void**)(((char*)vtable) + vtableindex);
#ifdef HOST_IA64
        cptr = vtable;
#else
        cptr = *vtable;
#endif
    }
    return cptr;
}
