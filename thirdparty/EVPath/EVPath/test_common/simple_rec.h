#ifndef SIMPLE_REC_H
#define SIMPLE_REC_H

#include <string.h>
#include "ffs.h"

#ifdef HAVE_WINDOWS_H
#define drand48() (((double)rand())/((double)RAND_MAX))
#define lrand48() rand()
#define srand48(x)
#endif

typedef struct _complex_rec {
    double r;
    double i;
} complex, *complex_ptr;

typedef struct _nested_rec {
    complex item;
} nested, *nested_ptr;

extern FMField nested_field_list[];
extern FMField complex_field_list[];

typedef struct _simple_rec {
    int integer_field;
    short short_field;
    long long_field;
    nested nested_field;
    double double_field;
    char char_field;
    int scan_sum;
} simple_rec, *simple_rec_ptr;

extern FMField simple_field_list[];
extern FMStructDescRec simple_format_list[];

extern void generate_simple_record(simple_rec_ptr event);

extern int verify_simple_record(simple_rec_ptr event);

/* checksum_simple_record - like verify but with verbose output */
#include <atl.h>
extern int checksum_simple_record(simple_rec_ptr event, attr_list attrs, int quiet);

#endif /* SIMPLE_REC_H */
