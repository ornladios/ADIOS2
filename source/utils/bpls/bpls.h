/*
 * ADIOS is freely available under the terms of the BSD license described
 * in the COPYING file in the top level directory of this source distribution.
 *
 * Copyright (c) 2008 - 2009.  UT-BATTELLE, LLC. All rights reserved.
 */

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h"

#include <map>

namespace adios2
{
namespace utils
{

/* definitions for bpls.c */
#define myfree(p)                                                              \
    if (p)                                                                     \
    {                                                                          \
        free(p);                                                               \
        p = NULL;                                                              \
    }

#define CUT_TO_BYTE(x) (x < 0 ? 0 : (x > 255 ? 255 : x))

#define MAX_DIMS 16
#define MAX_MASKS 10
#define MAX_BUFFERSIZE (10 * 1024 * 1024)

/* global defines needed for the type creation/setup functions */
enum ADIOS_DATATYPES
{
    adios_unknown = -1,
    adios_byte = 0,
    adios_short = 1,
    adios_integer = 2,
    adios_long = 4,
    adios_unsigned_byte = 50,
    adios_unsigned_short = 51,
    adios_unsigned_integer = 52,
    adios_unsigned_long = 54,
    adios_real = 5,
    adios_double = 6,
    adios_long_double = 7,
    adios_string = 9,
    adios_complex = 10,
    adios_double_complex = 11,
    adios_string_array = 12,
    adios_long_double_complex = 13
};

struct Entry
{
    bool isVar;
    std::string typeName;
    unsigned int typeIndex;
    Entry(bool b, std::string name, unsigned idx)
    : isVar(b), typeName(name), typeIndex(idx)
    {
    }
};

// how to print one data item of an array
// enum PrintDataType {STRING, INT, FLOAT, DOUBLE, COMPLEX};

char *mystrndup(const char *s, size_t n);
void init_globals();
void processDimSpecs();
void parseDimSpec(const std::string &str, int64_t *dims);
int compile_regexp_masks(void);
void printSettings(void);
int doList(const char *path);
void mergeLists(int nV, char **listV, int nA, char **listA, char **mlist,
                bool *isVar);

enum ADIOS_DATATYPES type_to_enum(std::string type);

template <class T>
int printVariableInfo(core::Engine *fp, core::IO *io,
                      core::Variable<T> *variable);

template <class T>
int readVar(core::Engine *fp, core::IO *io, core::Variable<T> *variable);

template <class T>
int readVarBlock(core::Engine *fp, core::IO *io, core::Variable<T> *variable,
                 int blockid);

template <class T>
size_t relative_to_absolute_step(core::Variable<T> *variable,
                                 const size_t relstep);
template <class T>
Dims get_global_array_signature(core::Engine *fp, core::IO *io,
                                core::Variable<T> *variable);
template <class T>
std::pair<size_t, Dims> get_local_array_signature(core::Engine *fp,
                                                  core::IO *io,
                                                  core::Variable<T> *variable);

int cmpstringp(const void *p1, const void *p2);
bool grpMatchesMask(char *name);
bool matchesAMask(const char *name);
int print_start(const std::string &fnamestr);
void print_slice_info(core::VariableBase *variable, bool timed, uint64_t *s,
                      uint64_t *c, Dims count);
int print_data(const void *data, int item, enum ADIOS_DATATYPES adiosvartypes,
               bool allowformat);

/* s is a character array not necessarily null terminated.
 * return false on OK print, true if it not XML (not printed)*/
bool print_data_xml(const char *s, const size_t length);

int print_dataset(const void *data, const std::string vartype, uint64_t *s,
                  uint64_t *c, int tdims, int *ndigits);
void print_endline(void);
void print_stop(void);
int print_data_hist(core::VariableBase *vi, char *varname);
int print_data_characteristics(void *min, void *max, double *avg,
                               double *std_dev,
                               enum ADIOS_DATATYPES adiosvartypes,
                               bool allowformat);

template <class T>
void print_decomp(core::Engine *fp, core::IO *io, core::Variable<T> *variable);

// close namespace
}
}
