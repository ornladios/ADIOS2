/*
 * ADIOS is freely available under the terms of the BSD license described
 * in the COPYING file in the top level directory of this source distribution.
 *
 * Copyright (c) 2008 - 2009.  UT-BATTELLE, LLC. All rights reserved.
 */

/*
 * List the content of a BP file. Rewritten from adios 1.x bpls C code with
 *adios 2.x C++ API
 *
 * Author: Norbert Podhorszki, pnorbert@ornl.gov
 *
 **/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

//#include <iomanip>
//#include <iostream>
#include <cinttypes>
#include <cstdio>
#include <string>
#include <vector>

#include "./bpls.h"

#include <errno.h>

#if defined(__GNUC__) && !(defined(__ICC) || defined(__INTEL_COMPILER))
#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) < 40900
/* pre GCC 4.9 cannot handle the C++ regex implementation. Will use C-lib
 * regex"*/
#define USE_C_REGEX
#endif
#endif

#ifdef USE_C_REGEX
#include <regex.h> // regular expression matching
#else
#include <regex>
#endif

#ifdef _WIN32
#include "shlwapi.h"
#include "windows.h"
#pragma comment(lib, "shlwapi.lib")
#pragma warning(disable : 4101) // unreferenced local variable
#else
#include <fnmatch.h>
#endif

#include "adios2sys/CommandLineArguments.hxx"

namespace adios2
{
namespace utils
{

using EntryMap = std::map<std::string, Entry>;

// global variables
// Values from the arguments or defaults

// output files' starting path (can be extended with subdirs,
// names, indexes)
std::string outpath;
char *varmask[MAX_MASKS]; // can have many -var masks (either shell patterns or
                          // extended regular expressions)
int nmasks;               // number of masks specified
char *vfile;              // file name to bpls
std::string start;        // dimension spec starting points
std::string count;        // dimension spec counts
std::string format;       // format string for one data element (e.g. %6.2f)

// Flags from arguments or defaults
bool dump; // dump data not just list info
bool output_xml;
bool use_regexp; // use varmasks as regular expressions
bool sortnames;  // sort names before listing
bool listattrs;  // do list attributes too
bool listmeshes; // do list meshes too
bool attrsonly;  // do list attributes only
bool readattrs;  // also read all attributes and print
bool longopt;    // -l is turned on
bool timestep;
bool noindex;          // do no print array indices with data
bool printByteAsChar;  // print 8 bit integer arrays as string
bool plot;             // dump histogram related information
bool hidden_attrs;     // show hidden attrs in BP file
int hidden_attrs_flag; // to be passed on in option struct
bool show_decomp;      // show decomposition of arrays

// other global variables
char *prgname; /* argv[0] */
// long timefrom, timeto;
int64_t istart[MAX_DIMS], icount[MAX_DIMS]; // negative values are allowed
int ndimsspecified = 0;
#ifdef USE_C_REGEX
regex_t varregex[MAX_MASKS]; // compiled regular expressions of varmask
#else
std::vector<std::regex> varregex;
#endif
int ncols = 6; // how many values to print in one row (only for -p)
int verbose = 0;
FILE *outf; // file to print to or stdout
char commentchar;

// help function
void display_help()
{
    // printf( "Usage: %s  \n", prgname);
    printf(
        "usage: bpls [OPTIONS] file [mask1 mask2 ...]\n"
        "\nList/dump content of a BP/HDF5 file. \n"
        "A mask can be a shell pattern like with 'ls' e.g. \"*/x?\".\n"
        "Variables with multiple timesteps are reported with an extra "
        "dimensions.\n"
        "The time dimension is the first dimension then.\n"
        "\n"
        "  --long      | -l           Print values of all scalars and "
        "attributes and\n"
        "                               min/max values of arrays (no overhead "
        "to get them!)\n"
        "  --attrs     | -a           List/match attributes too\n"
        "  --attrsonly | -A           List attributes only\n"
        "  --meshes    | -m           List meshes\n"
        /*
        "  --sort      | -r           Sort names before listing\n"
        */
        "  --timestep  | -t           Print values of timestep elements\n"
        "  --dump      | -d           Dump matched variables/attributes\n"
        "                               To match attributes too, add option "
        "-a\n"
        "  --regexp    | -e           Treat masks as extended regular "
        "expressions\n"
        "  --plot      | -p           Dumps the histogram information that can "
        "be read by gnuplot\n"
        "  --output    | -o <path>    Print to a file instead of stdout\n"
        /*
           "  --xml    | -x            # print as xml instead of ascii text\n"
         */
        "  --start     | -s \"spec\"    Offset indices in each dimension \n"
        "                               (default is 0 for all dimensions) \n"
        "                               <0 is handled as in python (-1 is "
        "last)\n"
        "  --count     | -c \"spec\"    Number of elements in each dimension\n"
        "                               -1 denotes 'until end' of dimension\n"
        "                               (default is -1 for all dimensions)\n"
        "  --noindex   | -y           Print data without array indices\n"
        "  --string    | -S           Print 8bit integer arrays as strings\n"
        "  --columns   | -n \"cols\"    Number of data elements per row to "
        "print\n"
        "  --format    | -f \"str\"     Format string to use for one data item "
        "in print\n"
        "                               instead of the default. E.g. "
        "\"%%6.3f\"\n"
        "  --hidden_attrs             Show hidden ADIOS attributes in the "
        "file\n"
        "  --decomp    | -D           Show decomposition of variables as layed "
        "out in file\n"
        /*
           "  --time    | -t N [M]      # print data for timesteps N..M only (or
           only N)\n"
           "                              default is to print all available
           timesteps\n"
         */
        "\n"
        "  Examples for slicing:\n"
        "  -s \"0,0,0\"   -c \"1,99,1\":  Print 100 elements (of the 2nd "
        "dimension).\n"
        "  -s \"0,0\"     -c \"1,-1\":    Print the whole 2nd dimension "
        "however large it is.\n"
        "  -s \"-1,-1\"   -c \"1,1\":     Print the very last element (of a 2D "
        "array)\n"
        "\n"
        "Help options\n"
        "  --help      | -h           Print this help.\n"
        "  --verbose   | -v           Print log about what this program is "
        "doing.\n"
        "                               Use multiple -v to increase logging "
        "level.\n"
        "Typical use: bpls -lav <file>\n");
}

bool option_help_was_called = false;
int optioncb_help(const char *argument, const char *value, void *call_data)
{
    // adios2sys::CommandLineArguments *arg =
    // static_cast<adios2sys::CommandLineArguments *>(call_data);
    // printf("%s\n", arg->GetHelp());
    display_help();
    option_help_was_called = true;
    return 1;
}

int optioncb_verbose(const char *argument, const char *value, void *call_data)
{
    verbose++;
    return 1;
}

int process_unused_args(adios2sys::CommandLineArguments &arg)
{
    int nuargs;
    char **uargs;
    arg.GetUnusedArguments(&nuargs, &uargs);

    std::vector<char *> retry_args;
    retry_args.push_back(new char[4]);

    // first arg is argv[0], so skip that
    for (int i = 1; i < nuargs; i++)
    {
        if (uargs[i] != NULL && uargs[i][0] == '-')
        {
            if (uargs[i][1] == '-')
            {
                fprintf(stderr, "Unknown long option: %s\n", uargs[i]);
                arg.DeleteRemainingArguments(nuargs, &uargs);
                return 1;
            }
            else
            {
                // Maybe -abc is -a -b -c?
                size_t len = strlen(uargs[i]);
                for (size_t j = 1; j < len; ++j)
                {
                    char *opt = new char[3];
                    opt[0] = '-';
                    opt[1] = uargs[i][j];
                    opt[2] = '\0';
                    retry_args.push_back(opt);
                }
            }
        }
        else if (vfile == NULL)
        {
            vfile = mystrndup(uargs[i], 4096);
            // fprintf(stderr, "Set file argument: %s\n", vfile);
        }
        else
        {
            varmask[nmasks] = mystrndup(uargs[i], 256);
            // fprintf(stderr, "Set mask %d argument: %s\n", nmasks,
            //        varmask[nmasks]);
            nmasks++;
        }
    }
    arg.DeleteRemainingArguments(nuargs, &uargs);

    if (retry_args.size() > 1)
    {
        // Run a new parse on the -a single letter arguments
        // fprintf(stderr, "Rerun parse on %zu options\n", retry_args.size());
        arg.Initialize(static_cast<int>(retry_args.size()), retry_args.data());
        arg.StoreUnusedArguments(false);
        if (!arg.Parse())
        {
            fprintf(stderr, "Parsing arguments failed\n");
            return 1;
        }
        for (size_t j = 0; j < retry_args.size(); ++j)
        {
            delete[] retry_args[j];
        }
    }
    else
    {
        delete[] retry_args[0];
    }

    return 0;
}

/** Main */
int bplsMain(int argc, char *argv[])
{
    int retval = 0;

    init_globals();

    adios2sys::CommandLineArguments arg;
    arg.Initialize(argc, argv);
    typedef adios2sys::CommandLineArguments argT;
    arg.StoreUnusedArguments(true);
    arg.AddCallback("--help", argT::NO_ARGUMENT, optioncb_help, &arg, "Help");
    arg.AddCallback("-h", argT::NO_ARGUMENT, optioncb_help, &arg, "");
    arg.AddCallback("--verbose", argT::NO_ARGUMENT, optioncb_verbose, nullptr,
                    "Print information about what bpls is doing");
    arg.AddCallback("-v", argT::NO_ARGUMENT, optioncb_verbose, nullptr, "");
    arg.AddBooleanArgument("--dump", &dump,
                           "Dump matched variables/attributes");
    arg.AddBooleanArgument("-d", &dump, "");
    arg.AddBooleanArgument(
        "--long", &longopt,
        "Print values of all scalars and attributes and min/max "
        "values of arrays");
    arg.AddBooleanArgument("-l", &longopt, "");
    arg.AddBooleanArgument("--regexp", &use_regexp,
                           "| -e Treat masks as extended regular expressions");
    arg.AddBooleanArgument("-e", &use_regexp, "");
    arg.AddArgument("--output", argT::SPACE_ARGUMENT, &outpath,
                    "| -o opt    Print to a file instead of stdout");
    arg.AddArgument("-o", argT::SPACE_ARGUMENT, &outpath, "");
    arg.AddArgument("--start", argT::SPACE_ARGUMENT, &start,
                    "| -s opt    Offset indices in each dimension (default is "
                    "0 for all dimensions).  opt<0 is handled as in python (-1 "
                    "is last)");
    arg.AddArgument("-s", argT::SPACE_ARGUMENT, &start, "");
    arg.AddArgument("--count", argT::SPACE_ARGUMENT, &count,
                    "| -c opt    Number of elements in each dimension. -1 "
                    "denotes 'until end' of dimension. default is -1 for all "
                    "dimensions");
    arg.AddArgument("-c", argT::SPACE_ARGUMENT, &count, "");
    arg.AddBooleanArgument("--noindex", &noindex,
                           " | -y Print data without array indices");
    arg.AddBooleanArgument("-y", &noindex, "");
    arg.AddBooleanArgument("--timestep", &timestep,
                           " | -t Print values of timestep elements");
    arg.AddBooleanArgument("-t", &timestep, "");
    arg.AddBooleanArgument("--attrs", &listattrs,
                           " | -a List/match attributes too");
    arg.AddBooleanArgument("-a", &listattrs, "");
    arg.AddBooleanArgument("--attrsonly", &attrsonly,
                           " | -A List/match attributes only (no variables)");
    arg.AddBooleanArgument("-A", &attrsonly, "");
    arg.AddBooleanArgument("--meshes", &listmeshes, " | -m List meshes");
    arg.AddBooleanArgument("-m", &listmeshes, "");
    arg.AddBooleanArgument("--string", &printByteAsChar,
                           " | -S Print 8bit integer arrays as strings");
    arg.AddBooleanArgument("-S", &printByteAsChar, "");
    arg.AddArgument("--columns", argT::SPACE_ARGUMENT, &ncols,
                    "| -n opt    Number of data elements per row to print");
    arg.AddArgument("-n", argT::SPACE_ARGUMENT, &ncols, "");
    arg.AddArgument("--format", argT::SPACE_ARGUMENT, &format,
                    "| -f opt    Format string to use for one data item ");
    arg.AddArgument("-f", argT::SPACE_ARGUMENT, &format, "");
    arg.AddBooleanArgument("--hidden_attrs", &hidden_attrs,
                           "  Show hidden ADIOS attributes in the file");
    arg.AddBooleanArgument(
        "--decompose", &show_decomp,
        "| -D Show decomposition of variables as layed out in file");
    arg.AddBooleanArgument("-D", &show_decomp, "");

    if (!arg.Parse())
    {
        fprintf(stderr, "Parsing arguments failed\n");
        return 1;
    }
    if (option_help_was_called)
        return 0;

    retval = process_unused_args(arg);
    if (retval)
    {
        return retval;
    }
    if (option_help_was_called)
        return 0;

    /* Check if we have a file defined */
    if (vfile == NULL)
    {
        fprintf(stderr, "Missing file name\n");
        return 1;
    }

    /* Process dimension specifications */
    parseDimSpec(start, istart);
    parseDimSpec(count, icount);

    // process the regular expressions
    if (use_regexp)
    {
        retval = compile_regexp_masks();
        if (retval)
            return retval;
    }

    if (noindex)
        commentchar = ';';
    else
        commentchar = ' ';

    if (hidden_attrs_flag)
        hidden_attrs = true;

    if (verbose > 1)
        printSettings();

    retval = print_start(outpath);
    if (retval)
        return retval;

    /* Start working */
    retval = doList(vfile);

    print_stop();

    /* Free allocated memories */
    for (int i = 0; i < nmasks; i++)
    {
        myfree(varmask[i]);
#ifdef USE_C_REGEX
        regfree(&(varregex[i]));
#else
        varregex.clear();
#endif
    }
    myfree(vfile);

    return retval;
}

void init_globals()
{
    int i;
    // variables for arguments
    for (i = 0; i < MAX_MASKS; i++)
        varmask[i] = NULL;
    nmasks = 0;
    vfile = NULL;
    verbose = 0;
    ncols = 6; // by default when printing ascii, print "X Y", not X: Y1 Y2...
    dump = false;
    output_xml = false;
    noindex = false;
    timestep = false;
    sortnames = false;
    listattrs = false;
    listmeshes = false;
    attrsonly = false;
    readattrs = false;
    longopt = false;
    // timefrom             = 1;
    // timeto               = -1;
    use_regexp = false;
    plot = false;
    hidden_attrs = false;
    hidden_attrs_flag = 0;
    printByteAsChar = false;
    show_decomp = false;
    for (i = 0; i < MAX_DIMS; i++)
    {
        istart[i] = 0LL;
        icount[i] = -1LL; // read full var by default
    }
    ndimsspecified = 0;
}

#define PRINT_DIMS_INT(str, v, n, loopvar)                                     \
    printf("%s = { ", str);                                                    \
    for (loopvar = 0; loopvar < n; loopvar++)                                  \
        printf("%d ", v[loopvar]);                                             \
    printf("}")

#define PRINT_DIMS_UINT64(str, v, n, loopvar)                                  \
    printf("%s = { ", str);                                                    \
    for (loopvar = 0; loopvar < n; loopvar++)                                  \
        printf("%" PRIu64 " ", v[loopvar]);                                    \
    printf("}")

#define PRINT_DIMS_INT64(str, v, n, loopvar)                                   \
    printf("%s = { ", str);                                                    \
    for (loopvar = 0; loopvar < n; loopvar++)                                  \
        printf("%" PRId64 " ", v[loopvar]);                                    \
    printf("}")

#define PRINT_DIMS_SIZET(str, v, n, loopvar)                                   \
    printf("%s = { ", str);                                                    \
    for (loopvar = 0; loopvar < n; loopvar++)                                  \
        printf("%zu ", v[loopvar]);                                            \
    printf("}")

void printSettings(void)
{
    int i;
    printf("Settings :\n");
    printf("  masks  : %d ", nmasks);
    for (i = 0; i < nmasks; i++)
        printf("%s ", varmask[i]);
    printf("\n");
    printf("  file   : %s\n", vfile);
    printf("  output : %s\n", (outpath.empty() ? "stdout" : outpath.c_str()));

    if (start.size())
    {
        PRINT_DIMS_INT64("  start", istart, ndimsspecified, i);
        printf("\n");
    }
    if (count.size())
    {
        PRINT_DIMS_INT64("  count", icount, ndimsspecified, i);
        printf("\n");
    }

    if (longopt)
        printf("      -l : show scalar values and min/max/avg of arrays\n");
    if (sortnames)
        printf("      -r : sort names before listing\n");
    if (attrsonly)
        printf("      -A : list attributes only\n");
    else if (listattrs)
        printf("      -a : list attributes too\n");
    else if (listmeshes)
        printf("      -m : list meshes too\n");
    if (dump)
        printf("      -d : dump matching variables and attributes\n");
    if (use_regexp)
        printf("      -e : handle masks as regular expressions\n");
    if (format.size())
        printf("      -f : dump using printf format \"%s\"\n", format.c_str());
    if (output_xml)
        printf("      -x : output data in XML format\n");
    if (show_decomp)
        printf("      -D : show decomposition of variables in the file\n");
    if (hidden_attrs)
    {
        printf("         : show hidden attributes in the file\n");
    }
}

void bpexit(int code, core::Engine *fp)
{
    if (fp != nullptr)
        fp->Close();
    exit(code);
}

void print_file_size(uint64_t size)
{
    static const char *sm[] = {"bytes", "KB", "MB", "GB", "TB", "PB", "EB"};
    uint64_t s = size, r = 0;
    int idx = 0;
    while (s / 1024 > 0)
    {
        r = s % 1024;
        s = s / 1024;
        idx++;
    }
    if (r > 511)
        s++;
    printf("  file size:     %" PRIu64 " %s\n", s, sm[idx]);
}

static inline int ndigits(size_t n)
{
    static char digitstr[32];
    return snprintf(digitstr, 32, "%zu", n);
}

int nEntriesMatched = 0;

int doList_vars(core::Engine *fp, core::IO *io)
{
    const core::DataMap &variables = io->GetVariablesDataMap();
    const core::DataMap &attributes = io->GetAttributesDataMap();

    // make a sorted list of all variables and attributes
    EntryMap entries;
    if (!attrsonly)
    {
        for (const auto &vpair : variables)
        {
            Entry e(true, vpair.second.first, vpair.second.second);
            entries.emplace(vpair.first, e);
        }
    }
    if (listattrs)
    {
        for (const auto &apair : attributes)
        {
            Entry e(false, apair.second.first, apair.second.second);
            entries.emplace(apair.first, e);
        }
    }

    // size_t nNames = entries.size();

    // calculate max length of variable names and type names in the first round
    int maxlen = 4; // need int for printf formatting
    int maxtypelen = 7;
    for (const auto &entrypair : entries)
    {
        int len = static_cast<int>(entrypair.first.size());
        if (len > maxlen)
            maxlen = len;
        len = static_cast<int>(entrypair.second.typeName.size());
        if (len > maxtypelen)
            maxtypelen = len;
    }

    /* VARIABLES */
    for (const auto &entrypair : entries)
    {
        int retval = 0;
        const std::string &name = entrypair.first;
        const Entry &entry = entrypair.second;
        bool matches = matchesAMask(name.c_str());
        if (matches)
        {
            nEntriesMatched++;

            // print definition of variable
            fprintf(outf, "%c %-*s  %-*s", commentchar, maxtypelen,
                    entry.typeName.c_str(), maxlen, name.c_str());
            if (!entry.isVar)
            {
                // list (and print) attribute
                if (readattrs || dump)
                {
                    fprintf(outf, "  attr   = ");
                    /*
                    int type_size = adios_type_size(vartype, value);
                    int nelems = attrsize / type_size;
                    char *p = (char *)value;
                    if (nelems > 1)
                        fprintf(outf, "{");
                    for (i = 0; i < nelems; i++)
                    {
                        if (i > 0)
                            fprintf(outf, ", ");
                        print_data(p, 0, vartype, false);
                        p += type_size;
                    }
                    if (nelems > 1)
                        fprintf(outf, "}");
                    */
                    fprintf(outf, "\n");
                    matches = false; // already printed
                }
                else
                {
                    fprintf(outf, "  attr\n");
                }
            }
            else
            {
                if (entry.typeName == "compound")
                {
                    // not supported
                }
#define declare_template_instantiation(T)                                      \
    else if (entry.typeName == helper::GetType<T>())                           \
    {                                                                          \
        core::Variable<T> *v = io->InquireVariable<T>(name);                   \
        retval = printVariableInfo(fp, io, v);                                 \
    }
                ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
            }
        }

        if (retval && retval != 10) // do not return after unsupported type
            return retval;
    }

    entries.clear();
    return 0;
}

template <class T>
int printVariableInfo(core::Engine *fp, core::IO *io,
                      core::Variable<T> *variable)
{
    size_t nsteps = variable->GetAvailableStepsCount();
    enum ADIOS_DATATYPES adiosvartype = type_to_enum(variable->m_Type);
    int retval = 0;

    if (!variable->m_SingleValue || nsteps > 1)
    {
        fprintf(outf, "  ");
        if (nsteps > 1)
            fprintf(outf, "%zu*", nsteps);
        if (variable->m_Shape.size() > 0)
        {
            fprintf(outf, "{%zu", variable->m_Shape[0]);
            for (size_t j = 1; j < variable->m_Shape.size(); j++)
            {
                fprintf(outf, ", %zu", variable->m_Shape[j]);
            }
            fprintf(outf, "}");
        }
        else
        {
            fprintf(outf, "scalar");
        }
#if 0
        if (longopt || plot)
        {
            adios_inq_var_stat(fp, vi, timestep && timed, show_decomp);
        }

        if (plot && vi->statistics && vi->statistics->histogram)
        {
            print_data_hist(vi, &names[n][1]);
        }

        if (longopt && vi->statistics)
        {

            if (timestep == false || timed == false)
            {

                fprintf(outf, " = ");
                if (vartype == adios_complex || vartype == adios_double_complex)
                {
                    // force printing (double,double) here
                    print_data(vi->statistics->min, 0, adios_double_complex,
                               false);
                    fprintf(outf, " / ");
                    print_data(vi->statistics->max, 0, adios_double_complex,
                               false);
                    fprintf(outf, " / ");
                    print_data(vi->statistics->avg, 0, adios_double_complex,
                               false);
                    fprintf(outf, " / ");
                    print_data(vi->statistics->std_dev, 0, adios_double_complex,
                               false);
                }
                else
                {
                    print_data(vi->statistics->min, 0, vartype, false);
                    fprintf(outf, " / ");
                    print_data(vi->statistics->max, 0, vartype, false);
                    fprintf(outf, " / ");
                    print_data(vi->statistics->avg, 0, adios_double, false);
                    fprintf(outf, " / ");
                    print_data(vi->statistics->std_dev, 0, adios_double, false);
                }

                // fprintf(outf," {MIN / MAX / AVG / STD_DEV} ");
            }
            else
            {
                int time_start = 0, time_end = vi->nsteps;

                if (start != NULL)
                {
                    if (istart[0] >= 0)
                        time_start = istart[0];
                    else
                        time_start = vi->nsteps - 1 + (int)istart[0];
                }

                if (count != NULL)
                {
                    if (icount[0] > 0)
                        time_end = time_start + (int)icount[0];
                    else
                        time_end = vi->nsteps + (int)icount[0] + 1;
                }

                if (time_start < 0 || time_start >= vi->nsteps)
                {
                    fprintf(stderr, "Error when reading variable %s. "
                                    "errno=%d : Variable (id=%d) has "
                                    "no data at %d time step\n",
                            names[n], 15, vi->varid, time_start);
                    bpexit(15, fp);
                }

                if (time_end < 0 || time_end > vi->nsteps)
                {
                    fprintf(stderr, "Error when reading variable %s. "
                                    "errno=%d : Variable (id=%d) has "
                                    "no data at %d time step\n",
                            names[n], 15, vi->varid, time_end);
                    bpexit(16, fp);
                }

                static char *indent_char = " ";
                int indent_len = 11;

                /* Start - Print the headers of statistics first */
                fprintf(outf, "\n%-*s", indent_len + 7, indent_char);
                fprintf(outf, "%10s  ", "MIN");
                fprintf(outf, "%10s  ", "MAX");
                fprintf(outf, "%10s  ", "AVG");
                fprintf(outf, "%10s  ", "STD DEV");

                /* End - Print the headers of statistics first */

                void *min, *max, *avg, *std_dev;
                enum ADIOS_DATATYPES vt = vartype;
                struct ADIOS_STAT_STEP *s = vi->statistics->steps;
                if (vi->type == adios_complex ||
                    vi->type == adios_double_complex)
                    vt = adios_double;
                fprintf(outf, "\n%-*sglobal:", indent_len, indent_char);
                print_data_characteristics(
                    vi->statistics->min, vi->statistics->max,
                    vi->statistics->avg, vi->statistics->std_dev, vt, false);

                for (i = time_start; i < time_end; i++)
                {
                    min = max = avg = std_dev = 0;
                    if (s->maxs && s->maxs[i])
                        max = s->maxs[i];
                    if (s->mins && s->mins[i])
                        min = s->mins[i];
                    if (s->avgs && s->avgs[i])
                        avg = s->avgs[i];
                    if (s->std_devs && s->std_devs[i])
                        std_dev = s->std_devs[i];

                    // Align the output, previous lines has atleast
                    // (maxlen + strlen(names[n])) characters
                    // Better way to printf N spaces?
                    fprintf(outf, "\n%-*st%-5d:", indent_len, indent_char, i);
                    print_data_characteristics(min, max, avg, std_dev, vt,
                                               false);
                }
                fprintf(outf, "\n");
            }
        } // longopt && vi->statistics
#endif
        fprintf(outf, "\n");

        if (show_decomp)
        {
            // adios_inq_var_blockinfo(fp, vi);
            print_decomp(fp, io, variable);
        }
    }
    else
    {
        // scalar
        fprintf(outf, "  scalar");
        if (longopt)
        {
            fprintf(outf, " = ");
            print_data(&variable->m_Value, 0, adiosvartype, false);
        }
        fprintf(outf, "\n");

        /*if (show_decomp)
        {
            adios_inq_var_blockinfo(fp, vi);
            adios_inq_var_stat(fp, vi, false, show_decomp);
            print_decomp(fp, vi, names[n], timed);
        }*/
    }

    if (dump && !show_decomp)
    {
        // print variable content
        retval = readVar(fp, io, variable);
        fprintf(outf, "\n");
    }
    return retval;
}

#define PRINT_ARRAY(str, ndim, dims, loopvar, format)                          \
    fprintf(outf, "%s", str);                                                  \
    if (ndim > 0)                                                              \
    {                                                                          \
        fprintf(outf, "{%" #format, dims[0]);                                  \
        for (loopvar = 1; loopvar < ndim; loopvar++)                           \
        {                                                                      \
            fprintf(outf, ", %" #format, dims[loopvar]);                       \
        }                                                                      \
        fprintf(outf, "}\n");                                                  \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        fprintf(outf, "empty\n");                                              \
    }

#define PRINT_ARRAY64(str, ndim, dims, loopvar)                                \
    fprintf(outf, "%s", str);                                                  \
    if (ndim > 0)                                                              \
    {                                                                          \
        fprintf(outf, "{%" PRIu64, dims[0]);                                   \
        for (loopvar = 1; loopvar < ndim; loopvar++)                           \
        {                                                                      \
            fprintf(outf, ", %" PRIu64, dims[loopvar]);                        \
        }                                                                      \
        fprintf(outf, "}\n");                                                  \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        fprintf(outf, "empty\n");                                              \
    }

void printMeshes(core::Engine *fp)
{
    fprintf(outf, "Mesh info: is not implemented in adios 2.x at the moment\n");
    return;
    /*
    int meshid, i, j; // loop vars
    int mpi_comm_dummy = 0;
    if (fp->nmeshes == 0)
    {
        fprintf(outf, "Mesh info: There are no meshes defined in this file\n");
        return;
    }
    fprintf(outf, "Mesh info: \n");
    for (meshid = 0; meshid < fp->nmeshes; meshid++)
    {
        fprintf(outf, "  %s\n", fp->mesh_namelist[meshid]);
        ADIOS_MESH *mi = adios_inq_mesh_byid(fp, meshid);
        if (mi)
        {
            if (meshid != mi->id)
                fprintf(
                    outf,
                    "  bpls warning: meshid (=%d) != inquired mesh id (%d)\n",
                    meshid, mi->id);
            if (strcmp(fp->mesh_namelist[meshid], mi->name))
                fprintf(outf, "  bpls warning: mesh name in list (=\"%s\") != "
                              "inquired mesh name (\"%s\")\n",
                        fp->mesh_namelist[meshid], mi->name);
            if (mi->file_name)
            {
                IO &io = adios.DeclareIO("mesh");
                Engine &meshfp = io->Open(mi->file_name, Mode::Read);
                adios_complete_meshinfo(fp, meshfp, mi);
                meshfp.Close();
            }
            fprintf(outf, "    type:         ");

            switch (mi->type)
            {
            case ADIOS_MESH_UNIFORM:
                fprintf(outf, "uniform\n");
                PRINT_ARRAY64("    dimensions:   ", mi->uniform->num_dimensions,
                              mi->uniform->dimensions, j)
                if (mi->uniform->origins)
                {
                    PRINT_ARRAY("    origins:      ",
                                mi->uniform->num_dimensions,
                                mi->uniform->origins, j, g)
                }
                if (mi->uniform->spacings)
                {
                    PRINT_ARRAY("    spacings:     ",
                                mi->uniform->num_dimensions,
                                mi->uniform->spacings, j, g)
                }
                if (mi->uniform->maximums)
                {
                    PRINT_ARRAY("    maximums:     ",
                                mi->uniform->num_dimensions,
                                mi->uniform->maximums, j, g)
                }
                break;

            case ADIOS_MESH_RECTILINEAR:
                fprintf(outf, "rectilinear\n");
                PRINT_ARRAY64("    dimensions:   ",
                              mi->rectilinear->num_dimensions,
                              mi->rectilinear->dimensions, j)
                if (mi->rectilinear->use_single_var)
                {
                    fprintf(outf, "    coordinates:  single-var: \"%s\"\n",
                            mi->rectilinear->coordinates[0]);
                }
                else
                {
                    fprintf(outf, "    coordinates:  multi-var: \"%s\"",
                            mi->rectilinear->coordinates[0]);
                    for (i = 1; i < mi->rectilinear->num_dimensions; i++)
                    {
                        fprintf(outf, ", \"%s\"",
                                mi->rectilinear->coordinates[i]);
                    }
                    fprintf(outf, "\n");
                }
                break;

            case ADIOS_MESH_STRUCTURED:
                fprintf(outf, "structured\n");
                PRINT_ARRAY64("    dimensions:   ",
                              mi->structured->num_dimensions,
                              mi->structured->dimensions, j);
                if (mi->structured->use_single_var)
                {
                    fprintf(outf, "    points:       single-var: \"%s\"\n",
                            mi->structured->points[0]);
                }
                else
                {
                    fprintf(outf, "    points:       multi-var: \"%s\"",
                            mi->structured->points[0]);
                    for (i = 1; i < mi->structured->num_dimensions; i++)
                    {
                        fprintf(outf, ", \"%s\"", mi->structured->points[i]);
                    }
                    fprintf(outf, "\n");
                }
                fprintf(outf, "    nspaces:      %d\n",
                        mi->structured->nspaces);
                break;

            case ADIOS_MESH_UNSTRUCTURED:
                fprintf(outf, "unstructured\n");
                if (mi->unstructured->nvar_points <= 1)
                {
                    fprintf(outf, "    npoints:      %" PRIu64 "\n",
                            mi->unstructured->npoints);
                    fprintf(outf, "    points:       single-var: \"%s\"\n",
                            mi->unstructured->points[0]);
                }
                else
                {
                    fprintf(outf, "    points:       multi-var: \"%s\"",
                            mi->unstructured->points[0]);
                    for (i = 1; i < mi->unstructured->nvar_points; i++)
                    {
                        fprintf(outf, ", \"%s\"", mi->unstructured->points[i]);
                    }
                    fprintf(outf, "\n");
                }
                fprintf(outf, "    ncsets:       %d\n",
                        mi->unstructured->ncsets);
                for (i = 0; i < mi->unstructured->ncsets; i++)
                {
                    fprintf(outf, "    cell set %d:\n", i);
                    fprintf(outf, "      cell type:  %d\n",
                            mi->unstructured->ctypes[i]);
                    fprintf(outf, "      ncells:     %" PRIu64 "\n",
                            mi->unstructured->ccounts[i]);
                    fprintf(outf, "      cells var:  \"%s\"\n",
                            mi->unstructured->cdata[i]);
                }
                fprintf(outf, "    nspaces:      %d\n",
                        mi->unstructured->nspaces);
                break;

            default:
                fprintf(outf, "undefined\n");
            }
            fprintf(outf, "    time varying: %s\n",
                    (mi->time_varying ? "yes" : "no"));
            adios_free_meshinfo(mi);
        }
    }
    fprintf(outf, "\n");
    */
}

std::vector<std::string> getEnginesList(const std::string path)
{
    std::vector<std::string> list;
#ifdef ADIOS2_HAVE_HDF5
    size_t slen = path.length();
    if (slen >= 3 && path.compare(slen - 3, 3, ".h5") == 0)
    {
        list.push_back("HDF5");
        list.push_back("BPFile");
    }
    else
    {
        list.push_back("BPFile");
        list.push_back("HDF5");
    }
#else
    list.push_back("BPFile");
#endif
    return list;
}

int doList(const char *path)
{
    char init_params[128];
    int adios_verbose = 2;

    if (verbose > 1)
        printf("\nADIOS Open: read header info from %s\n", path);

    // initialize BP reader
    if (verbose > 1)
        adios_verbose = 3; // print info lines
    if (verbose > 2)
        adios_verbose = 4; // print debug lines
    sprintf(init_params, "verbose=%d", adios_verbose);
    if (hidden_attrs)
        strcat(init_params, ";show_hidden_attrs");

    core::ADIOS adios(true);
    core::IO &io = adios.DeclareIO("bpls");
    core::Engine *fp = nullptr;
    std::vector<std::string> engineList = getEnginesList(path);
    for (auto &engineName : engineList)
    {
        if (verbose > 2)
            printf("Try %s engine to open the file...\n", engineName.c_str());
        io.SetEngine(engineName);
        try
        {
            fp = &io.Open(path, Mode::Read);
        }
        catch (std::exception &e)
        {
            if (verbose > 2)
                printf("Failed to open with %s engine: %s\n",
                       engineName.c_str(), e.what());
        }
        if (fp != nullptr)
            break;
    }

    if (fp != nullptr)
    {
        //, variables, timesteps, and attributes
        // all parameters are integers,
        // besides the last parameter, which is an array of strings for holding
        // the
        // list of group names
        // ntsteps = fp->tidx_stop - fp->tidx_start + 1;
        if (verbose)
        {
            const std::map<std::string, Params> &variablesInfo =
                io.GetAvailableVariables();
            const std::map<std::string, Params> &attributesInfo =
                io.GetAvailableAttributes();
            printf("File info:\n");
            printf("  of variables:  %zu\n", variablesInfo.size());
            printf("  of attributes: %zu\n", attributesInfo.size());
            // printf("  of meshes:     %d\n", fp->nmeshes);
            // print_file_size(fp->file_size);
            // printf("  bp version:    %d\n", fp->version);
            // printf("  endianness:    %s\n",
            //       (fp->endianness ? "Big Endian" : "Little Endian"));
            if (longopt)
                printf("  statistics:    Min / Max \n");
            printf("\n");
        }

        // Print out the meshes in the file
        if (listmeshes)
        {
            printMeshes(fp);
        }

        doList_vars(fp, &io);

        if (nmasks > 0 && nEntriesMatched == 0)
        {
            fprintf(stderr,
                    "\nError: None of the variables/attributes matched any "
                    "name/regexp you provided\n");
            return 4;
        }
        fp->Close();
    }
    else
    {
        fprintf(stderr, "\nError: Could not open this file with any ADIOS2 "
                        "file reading engines\n");
        return 4;
    }
    return 0;
}

/*
int print_data_hist(ADIOS_VARINFO *vi, char *varname)
{
    char hist_file[256], gnuplot_file[256];
    int i;
    char xtics[512], str[512];
    FILE *out_hist, *out_plot;
    struct ADIOS_HIST *h = vi->statistics->histogram;

    memcpy(hist_file, varname, strlen(varname) + 1);
    strcat(hist_file, ".hist");

    if ((out_hist = fopen(hist_file, "w")) == NULL)
    {
        fprintf(stderr, "Error at opening for writing file %s: %s\n",
hist_file,
                strerror(errno));
        return 30;
    }

    memcpy(gnuplot_file, varname, strlen(varname) + 1);
    strcat(gnuplot_file, ".gpl");

    if ((out_plot = fopen(gnuplot_file, "w")) == NULL)
    {
        fprintf(stderr, "Error at opening for writing file %s: %s\n",
                gnuplot_file, strerror(errno));
        return 30;
    }

    xtics[0] = '\0';
    strcat(xtics, "set xtics offset start axis (");
    for (i = 0; i <= h->num_breaks; i++)
    {
        if (i == 0)
        {
            fprintf(out_hist, "-Inf %.2lf %u\n", h->breaks[i],
                    h->gfrequencies[i]);
            sprintf(str, "\"-Inf\" pos(%d)", i);
        }
        else if (i < h->num_breaks)
        {
            fprintf(out_hist, "%.2lf %.2lf %u\n", h->breaks[i - 1],
                    h->breaks[i], h->gfrequencies[i]);
            sprintf(str, ", \"%.2lf\" pos(%d)", h->breaks[i - 1], i);
        }
        else
        {
            fprintf(out_hist, "%.2lf Inf %u\n", h->breaks[i],
                    h->gfrequencies[i]);
            sprintf(str, ", \"Inf\" pos(%d)", i);
        }
        strcat(xtics, str);
    }
    strcat(xtics, ")\n");

    fprintf(out_plot, "start = -0.5\npos(x) = start + x * 1\nset boxwidth "
                      "1\nset style fill solid border 5#5lt6#6\n");
    fputs(xtics, out_plot);
    fprintf(out_plot, "plot '%s' using 3 smooth frequency w boxes\n",
            hist_file);
    fprintf(out_plot, "pause -1 'Press Enter to quit'\n");
    return 0;
}
*/

int cmpstringp(const void *p1, const void *p2)
{
    /* The actual arguments to this function are "pointers to
       pointers to char", but strcmp() arguments are "pointers
       to char", hence the following cast plus dereference */
    return strcmp(*(char *const *)p1, *(char *const *)p2);
}
/** Merge listV with listA if listattrs=true, return listA if
  attrsonly=true,
  otherwise just return listV.
  If sortnames=true, quicksort the result list.
 */
void mergeLists(int nV, char **listV, int nA, char **listA, char **mlist,
                bool *isVar)
{
    int v, a, idx;
    if (sortnames && listattrs && !attrsonly)
    {
        // merge sort the two lists
        v = 0;
        a = 0;
        while (v < nV || a < nA)
        {
            if (a < nA && (v >= nV || strcmp(listV[v], listA[a]) > 0))
            {
                // fully consumed var list or
                // next item in attr list is less than next item in var list
                mlist[v + a] = listA[a];
                isVar[v + a] = false;
                a++;
            }
            else
            {
                mlist[v + a] = listV[v];
                isVar[v + a] = true;
                v++;
            }
        }
    }
    else
    {
        // first add vars then attrs (if ask ed)
        idx = 0;
        if (!attrsonly)
        {
            for (v = 0; v < nV; v++)
            {
                mlist[idx] = listV[v];
                isVar[idx] = true;
                idx++;
            }
        }
        if (listattrs)
        {
            for (a = 0; a < nA; a++)
            {
                mlist[idx] = listA[a];
                isVar[idx] = false;
                idx++;
            }
        }
    }
}

int getTypeInfo(enum ADIOS_DATATYPES adiosvartype, int *elemsize)
{
    switch (adiosvartype)
    {
    case adios_unsigned_byte:
        *elemsize = 1;
        break;
    case adios_byte:
        *elemsize = 1;
        break;
    case adios_string:
        *elemsize = 1;
        break;

    case adios_unsigned_short:
        *elemsize = 2;
        break;
    case adios_short:
        *elemsize = 2;
        break;

    case adios_unsigned_integer:
        *elemsize = 4;
        break;
    case adios_integer:
        *elemsize = 4;
        break;

    case adios_unsigned_long:
        *elemsize = 8;
        break;
    case adios_long:
        *elemsize = 8;
        break;

    case adios_real:
        *elemsize = 4;
        break;

    case adios_double:
        *elemsize = 8;
        break;

    case adios_complex:
        *elemsize = 8;
        break;

    case adios_double_complex:
        *elemsize = 16;
        break;

    case adios_long_double: // do not know how to print
    //*elemsize = 16;
    default:
        return 1;
    }
    return 0;
}

/** Read data of a variable and print
 * Return: 0: ok, != 0 on error
 */
template <class T>
int readVar(core::Engine *fp, core::IO *io, core::Variable<T> *variable)
{
    int i, j;
    uint64_t start_t[MAX_DIMS],
        count_t[MAX_DIMS];             // processed <0 values in start/count
    uint64_t s[MAX_DIMS], c[MAX_DIMS]; // for block reading of smaller chunks
    int tdims;                         // number of dimensions including time
    int tidx;                          // 0 or 1 to account for time dimension
    uint64_t nelems;                   // number of elements to read
    // size_t elemsize;                   // size in bytes of one element
    uint64_t st, ct;
    T *data;
    uint64_t sum; // working var to sum up things
    uint64_t
        maxreadn; // max number of elements to read once up to a limit (10MB
                  // of
                  // data)
    uint64_t actualreadn;     // our decision how much to read at once
    uint64_t readn[MAX_DIMS]; // how big chunk to read in in each dimension?
    bool incdim;              // used in incremental reading in
    int ndigits_dims[32];     // # of digits (to print) of each dimension

    const size_t elemsize = variable->m_ElementSize;
    const int nsteps = static_cast<int>(variable->GetAvailableStepsCount());
    const int ndim = static_cast<int>(variable->m_Shape.size());
    // create the counter arrays with the appropriate lengths
    // transfer start and count arrays to format dependent arrays

    nelems = 1;
    tidx = 0;

    if (nsteps > 1)
    {
        if (istart[0] < 0) // negative index means last-|index|
            st = nsteps + istart[0];
        else
            st = istart[0];
        if (icount[0] < 0) // negative index means last-|index|+1-start
            ct = nsteps + icount[0] + 1 - st;
        else
            ct = icount[0];

        if (verbose > 2)
            printf("    j=0, st=%" PRIu64 " ct=%" PRIu64 "\n", st, ct);

        start_t[0] = st;
        count_t[0] = ct;
        nelems *= ct;
        if (verbose > 1)
            printf("    s[0]=%" PRIu64 ", c[0]=%" PRIu64 ", n=%" PRIu64 "\n",
                   start_t[0], count_t[0], nelems);

        tidx = 1;
    }
    tdims = ndim + tidx;

    for (j = 0; j < ndim; j++)
    {
        if (istart[j + tidx] < 0) // negative index means last-|index|
            st = variable->m_Shape[j] + istart[j + tidx];
        else
            st = istart[j + tidx];
        if (icount[j + tidx] < 0) // negative index means last-|index|+1-start
            ct = variable->m_Shape[j] + icount[j + tidx] + 1 - st;
        else
            ct = icount[j + tidx];

        if (verbose > 2)
            printf("    j=%d, st=%" PRIu64 " ct=%" PRIu64 "\n", j + tidx, st,
                   ct);

        start_t[j + tidx] = st;
        count_t[j + tidx] = ct;
        nelems *= ct;
        if (verbose > 1)
            printf("    s[%d]=%" PRIu64 ", c[%d]=%" PRIu64 ", n=%" PRIu64 "\n",
                   j + tidx, start_t[j + tidx], j + tidx, count_t[j + tidx],
                   nelems);
    }

    if (verbose > 1)
    {
        printf(" total size of data to read = %" PRIu64 "\n",
               nelems * elemsize);
    }

    print_slice_info(variable, start_t, count_t);

    maxreadn = (uint64_t)MAX_BUFFERSIZE / elemsize;
    if (nelems < maxreadn)
        maxreadn = nelems;

    // special case: string. Need to use different elemsize
    /*if (vi->type == adios_string)
    {
        if (vi->value)
            elemsize = strlen(vi->value) + 1;
        maxreadn = elemsize;
    }*/

    // allocate data array
    data = (T *)malloc(maxreadn * elemsize);

    // determine strategy how to read in:
    //  - at once
    //  - loop over 1st dimension
    //  - loop over 1st & 2nd dimension
    //  - etc
    if (verbose > 1)
        printf("Read size strategy:\n");
    sum = (uint64_t)1;
    actualreadn = (uint64_t)1;
    for (i = tdims - 1; i >= 0; i--)
    {
        if (sum >= (uint64_t)maxreadn)
        {
            readn[i] = 1;
        }
        else
        {
            readn[i] = maxreadn / (int)sum; // sum is small for 4 bytes here
            // this may be over the max count for this dimension
            if (readn[i] > count_t[i])
                readn[i] = count_t[i];
        }
        if (verbose > 1)
            printf("    dim %d: read %" PRIu64 " elements\n", i, readn[i]);
        sum = sum * (uint64_t)count_t[i];
        actualreadn = actualreadn * readn[i];
    }
    if (verbose > 1)
        printf("    read %" PRIu64 " elements at once, %" PRIu64
               " in total (nelems=%" PRIu64 ")\n",
               actualreadn, sum, nelems);

    // init s and c
    // and calculate ndigits_dims
    for (j = 0; j < tdims; j++)
    {
        s[j] = start_t[j];
        c[j] = readn[j];

        ndigits_dims[j] = ndigits(start_t[j] + count_t[j] -
                                  1); // -1: dim=100 results in 2 digits (0..99)
    }

    // read until read all 'nelems' elements
    sum = 0;
    while (sum < nelems)
    {

        // how many elements do we read in next?
        actualreadn = 1;
        for (j = 0; j < tdims; j++)
            actualreadn *= c[j];

        if (verbose > 2)
        {
            printf("adios_read_var name=%s ", variable->m_Name.c_str());
            PRINT_DIMS_UINT64("  start", s, tdims, j);
            PRINT_DIMS_UINT64("  count", c, tdims, j);
            printf("  read %" PRIu64 " elems\n", actualreadn);
        }

        // read a slice finally
        Dims startv = helper::Uint64ArrayToSizetVector(tdims - tidx, s + tidx);
        Dims countv = helper::Uint64ArrayToSizetVector(tdims - tidx, c + tidx);
        if (verbose > 2)
        {
            printf("set selection: ");
            PRINT_DIMS_SIZET("  start", startv.data(), tdims - tidx, j);
            PRINT_DIMS_SIZET("  count", countv.data(), tdims - tidx, j);
            printf("\n");
        }
        variable->SetSelection({startv, countv});

        if (nsteps > 1)
        {
            if (verbose > 2)
            {
                printf("set Step selection: from %" PRIu64 " read %" PRIu64
                       " steps\n",
                       s[0], c[0]);
            }
            variable->SetStepSelection({s[0], c[0]});
        }
        fp->Get(*variable, data, adios2::Mode::Sync);

        // print slice
        print_dataset(data, variable->m_Type, s, c, tdims, ndigits_dims);

        // prepare for next read
        sum += actualreadn;
        incdim = true; // largest dim should be increased
        for (j = tdims - 1; j >= 0; j--)
        {
            if (incdim)
            {
                if (s[j] + c[j] == start_t[j] + count_t[j])
                {
                    // reached the end of this dimension
                    s[j] = start_t[j];
                    c[j] = readn[j];
                    incdim = true; // next smaller dim can increase too
                }
                else
                {
                    // move up in this dimension up to total count
                    s[j] += readn[j];
                    if (s[j] + c[j] > start_t[j] + count_t[j])
                    {
                        // do not reach over the limit
                        c[j] = start_t[j] + count_t[j] - s[j];
                    }
                    incdim = false;
                }
            }
        }
    } // end while sum < nelems
    print_endline();

    free(data);
    return 0;
}

/** Read one writeblock of a variable and print
 * Return: 0: ok, != 0 on error
 */
template <class T>
int readVarBlock(core::Engine *fp, core::IO *io, core::Variable<T> *variable,
                 int blockid)
{
    int i, j;
    uint64_t start_t[MAX_DIMS],
        count_t[MAX_DIMS];             // processed <0 values in start/count
    uint64_t s[MAX_DIMS], c[MAX_DIMS]; // for block reading of smaller chunks
    uint64_t nelems;                   // number of elements to read
    int elemsize;                      // size in bytes of one element
    int tidx;
    uint64_t st, ct;
    void *data;
    uint64_t sum;    // working var to sum up things
    int maxreadn;    // max number of elements to read once up to a limit (10MB
                     // of
                     // data)
    int actualreadn; // our decision how much to read at once
    int readn[MAX_DIMS]; // how big chunk to read in in each dimension?
    int status;
    bool incdim;          // used in incremental reading in
    int ndigits_dims[32]; // # of digits (to print) of each dimension

#if 0
    if (getTypeInfo(vi->type, &elemsize))
    {
        fprintf(stderr, "Adios type %d (%s) not supported in bpls. var=%s\n",
                vi->type, adios_type_to_string(vi->type), name);
        return 10;
    }

    if (blockid < 0 || blockid > vi->sum_nblocks)
    {
        fprintf(stderr,
                "Invalid block id for var=%s, id=%d, available %d blocks\n",
                name, blockid, vi->sum_nblocks);
        return 10;
    }

    // create the counter arrays with the appropriate lengths
    // transfer start and count arrays to format dependent arrays

    nelems = 1;
    tidx = 0;

    if (timed)
    {
        if (istart[0] < 0) // negative index means last-|index|
            st = nsteps + istart[0];
        else
            st = istart[0];
        if (icount[0] < 0) // negative index means last-|index|+1-start
            ct = nsteps + icount[0] + 1 - st;
        else
            ct = icount[0];

        if (verbose > 2)
            printf("    time: st=%" PRIu64 " ct=%" PRIu64 "\n", st, ct);

        // check if this block falls into the requested time
        int idx = 0;
        for (i = 0; i < st; i++)
        {
            idx += vi->nblocks[i];
        }
        if (blockid < idx)
            return 0;
        for (i = st; i < st + ct; i++)
        {
            idx += vi->nblocks[i];
        }
        if (blockid > idx)
            return 0;
        tidx = 1;
    }

    int out_of_bound = 0;
    for (j = 0; j < vi->ndim; j++)
    {
        if (istart[j + tidx] < 0) // negative index means last-|index|
            st = vi->blockinfo[blockid].count[j] + istart[j + tidx];
        else
            st = istart[j + tidx];
        if (icount[j + tidx] < 0) // negative index means last-|index|+1-start
            ct = vi->blockinfo[blockid].count[j] + icount[j + tidx] + 1 - st;
        else
            ct = icount[j + tidx];

        if (st > vi->blockinfo[blockid].count[j])
        {
            out_of_bound = 1;
        }
        else if (ct > vi->blockinfo[blockid].count[j] - st)
        {
            ct = vi->blockinfo[blockid].count[j] - st;
        }
        if (verbose > 2)
            printf("    j=%d, st=%" PRIu64 " ct=%" PRIu64 "\n", j, st, ct);

        start_t[j] = st;
        count_t[j] = ct;
        nelems *= ct;
        if (verbose > 1)
            printf("    s[%d]=%" PRIu64 ", c[%d]=%" PRIu64 ", n=%" PRIu64 "\n",
                   j, start_t[j], j, count_t[j], nelems);
    }

    if (verbose > 1)
    {
        printf(" total size of data to read = %" PRIu64 "\n",
               nelems * elemsize);
    }

    print_slice_info(vi->ndim, vi->blockinfo[blockid].count, false, vi->nsteps,
                     start_t, count_t);

    if (out_of_bound)
        return 0;

    maxreadn = MAX_BUFFERSIZE / elemsize;
    if (nelems < maxreadn)
        maxreadn = nelems;

    // allocate data array
    data = (void *)malloc(maxreadn * elemsize + 8); // +8 for just to be sure

    // determine strategy how to read in:
    //  - at once
    //  - loop over 1st dimension
    //  - loop over 1st & 2nd dimension
    //  - etc
    if (verbose > 1)
        printf("Read size strategy:\n");
    sum = (uint64_t)1;
    actualreadn = (uint64_t)1;
    for (i = vi->ndim - 1; i >= 0; i--)
    {
        if (sum >= (uint64_t)maxreadn)
        {
            readn[i] = 1;
        }
        else
        {
            readn[i] = maxreadn / (int)sum; // sum is small for 4 bytes here
            // this may be over the max count for this dimension
            if (readn[i] > count_t[i])
                readn[i] = count_t[i];
        }
        if (verbose > 1)
            printf("    dim %d: read %d elements\n", i, readn[i]);
        sum = sum * (uint64_t)count_t[i];
        actualreadn = actualreadn * readn[i];
    }
    if (verbose > 1)
        printf("    read %d elements at once, %" PRIu64
               " in total (nelems=%" PRIu64 ")\n",
               actualreadn, sum, nelems);

    // init s and c
    // and calculate ndigits_dims
    for (j = 0; j < vi->ndim; j++)
    {
        s[j] = start_t[j];
        c[j] = readn[j];

        ndigits_dims[j] = ndigits(start_t[j] + count_t[j] -
                                  1); // -1: dim=100 results in 2 digits (0..99)
    }

    // read until read all 'nelems' elements
    sum = 0;
    while (sum < nelems)
    {

        // how many elements do we read in next?
        actualreadn = 1;
        for (j = 0; j < vi->ndim; j++)
            actualreadn *= c[j];

        uint64_t startoffset = s[vi->ndim - 1];
        uint64_t tmpprod = c[vi->ndim - 1];
        for (i = vi->ndim - 2; i >= 0; i--)
        {
            startoffset += s[i] * tmpprod;
            tmpprod *= c[i];
        }

        if (verbose > 2)
        {
            printf("adios_read_var name=%s ", name);
            PRINT_DIMS_UINT64("  start", s, vi->ndim, j);
            PRINT_DIMS_UINT64("  count", c, vi->ndim, j);
            printf("  read %d elems\n", actualreadn);
        }
        if (verbose > 1)
            printf("    read block %d from offset %" PRIu64 " nelems %d)\n",
                   blockid, startoffset, actualreadn);

        // read a slice finally
        ADIOS_SELECTION *wb = adios_selection_writeblock(blockid);
        wb->u.block.is_absolute_index = true;
        wb->u.block.is_sub_pg_selection = 1;
        wb->u.block.element_offset = startoffset;
        wb->u.block.nelements = actualreadn;
        status = adios_schedule_read_byid(fp, wb, vi->varid, 0, 1, data);

        if (status < 0)
        {
            fprintf(stderr, "Error when scheduling variable %s for reading. "
                            "errno=%d : %s \n",
                    name, adios_errno, adios_errmsg());
            free(data);
            return 11;
        }

        status = adios_perform_reads(fp, 1); // blocking read performed here
        adios_selection_delete(wb);
        if (status < 0)
        {
            fprintf(stderr, "Error when reading variable %s. errno=%d : %s \n",
                    name, adios_errno, adios_errmsg());
            free(data);
            return 11;
        }

        // print slice
        print_dataset(data, vi->type, s, c, vi->ndim, ndigits_dims);

        // prepare for next read
        sum += actualreadn;
        incdim = true; // largest dim should be increased
        for (j = vi->ndim - 1; j >= 0; j--)
        {
            if (incdim)
            {
                if (s[j] + c[j] == start_t[j] + count_t[j])
                {
                    // reached the end of this dimension
                    s[j] = start_t[j];
                    c[j] = readn[j];
                    incdim = true; // next smaller dim can increase too
                }
                else
                {
                    // move up in this dimension up to total count
                    s[j] += readn[j];
                    if (s[j] + c[j] > start_t[j] + count_t[j])
                    {
                        // do not reach over the limit
                        c[j] = start_t[j] + count_t[j] - s[j];
                    }
                    incdim = false;
                }
            }
        }
    } // end while sum < nelems
    print_endline();

    free(data);
#else
    printf("Block reading is not implemented yet\n");
#endif
    return 0;
}

bool matchesAMask(const char *name)
{
    int startpos = 0; // to match with starting / or without
#ifdef USE_C_REGEX
    regmatch_t pmatch[1] = {{(regoff_t)-1, (regoff_t)-1}};
#else
#endif

    if (nmasks == 0)
        return true;

    for (int i = 0; i < nmasks; i++)
    {
        if (use_regexp)
        {
#ifdef USE_C_REGEX
            int excode = regexec(&(varregex[i]), name, 1, pmatch, 0);
            if (name[0] == '/') // have to check if it matches from the second
                                // character too
                startpos = 1;
            if (excode == 0 && // matches
                (pmatch[0].rm_so == 0 ||
                 pmatch[0].rm_so == startpos) && // from the beginning
                pmatch[0].rm_eo == strlen(name)  // to the very end of the name
                )
#else
            bool matches = std::regex_match(name, varregex[i]);
            if (!matches && name[0] == '/')
                matches = std::regex_match(name + 1, varregex[i]);
            if (matches)
#endif
            {
                if (verbose > 1)
                    printf("Name %s matches regexp %i %s\n", name, i,
                           varmask[i]);
                return true;
            }
        }
        else
        {
            // use shell pattern matching
            if (varmask[i][0] != '/' && name[0] == '/')
                startpos = 1;
#ifdef _WIN32
            if (PathMatchSpec(name + startpos, varmask[i]))
#else
            if (fnmatch(varmask[i], name + startpos, FNM_FILE_NAME) == 0)
#endif
            {
                if (verbose > 1)
                    printf("Name %s matches varmask %i %s\n", name, i,
                           varmask[i]);
                return true;
            }
        }
    }
    return false;
}

int print_start(const std::string &fname)
{
    if (fname.empty())
    {
        outf = stdout;
    }
    else
    {
        if ((outf = fopen(fname.c_str(), "w")) == NULL)
        {
            fprintf(stderr, "Error at opening for writing file %s: %s\n",
                    fname.c_str(), strerror(errno));
            return 30;
        }
    }
    return 0;
}

void print_stop() { fclose(outf); }

static int nextcol =
    0; // column index to start with (can have lines split in two calls)

void print_slice_info(core::VariableBase *variable, uint64_t *s, uint64_t *c)
{
    // print the slice info in indexing is on and
    // not the complete variable is read
    size_t ndim = variable->m_Shape.size();
    size_t nsteps = variable->m_AvailableStepsCount;
    bool isaslice = false;
    int tidx = (nsteps > 1 ? 1 : 0);
    size_t tdim = ndim + tidx;
    if (nsteps > 1)
    {
        if (c[0] < nsteps)
            isaslice = true;
    }
    for (size_t i = 0; i < ndim; i++)
    {
        if (c[i + tidx] < variable->m_Shape[i])
            isaslice = true;
    }
    if (isaslice)
    {
        fprintf(outf, "%c   slice (%" PRIu64 ":%" PRIu64, commentchar, s[0],
                s[0] + c[0] - 1);
        for (size_t i = 1; i < tdim; i++)
        {
            fprintf(outf, ", %" PRIu64 ":%" PRIu64, s[i], s[i] + c[i] - 1);
        }
        fprintf(outf, ")\n");
    }
}

const std::map<std::string, enum ADIOS_DATATYPES> adios_types_map = {
    {"char", adios_unsigned_byte},
    {"int", adios_integer},
    {"float", adios_real},
    {"double", adios_double},
    {"float complex", adios_complex},
    {"double complex", adios_double_complex},
    {"signed char", adios_byte},
    {"short", adios_short},
    {"long int", adios_long},
    {"long long int", adios_long},
    {"string", adios_string},
    {"string array", adios_string_array},
    {"unsigned char", adios_unsigned_byte},
    {"unsigned short", adios_unsigned_short},
    {"unsigned int", adios_unsigned_integer},
    {"unsigned long int", adios_unsigned_long},
    {"unsigned long long int", adios_unsigned_long}};

enum ADIOS_DATATYPES type_to_enum(std::string type)
{
    auto itType = adios_types_map.find(type);
    if (itType == adios_types_map.end())
    {
        return adios_unknown;
    }
    return itType->second;
}

int print_data_as_string(const void *data, int maxlen,
                         enum ADIOS_DATATYPES adiosvartype)
{
    const char *str = (const char *)data;
    int len = maxlen;
    switch (adiosvartype)
    {
    case adios_unsigned_byte:
    case adios_byte:
    case adios_string:
        while (str[len - 1] == 0)
        {
            len--;
        } // go backwards on ascii 0s
        if (len < maxlen)
        {
            // it's a C string with terminating \0
            fprintf(outf, "\"%s\"", str);
        }
        else
        {
            // fortran VARCHAR, lets trim from right padded zeros
            while (str[len - 1] == ' ')
            {
                len--;
            }
            fprintf(outf, "\"%*.*s\"", len, len, (char *)data);
            if (len < maxlen)
                fprintf(outf, " + %d spaces", maxlen - len);
        }
        break;
    default:
        fprintf(stderr, "Error in bpls code: cannot use print_data_as_string() "
                        "for type \"%d\"\n",
                adiosvartype);
        return -1;
        break;
    }
    return 0;
}

int print_data_characteristics(void *min, void *max, double *avg,
                               double *std_dev,
                               enum ADIOS_DATATYPES adiosvartype,
                               bool allowformat)
{
    bool f = format.size() && allowformat;
    const char *fmt = format.c_str();

    switch (adiosvartype)
    {
    case adios_unsigned_byte:
        if (min)
            fprintf(outf, (f ? fmt : "%10hhu  "), *((unsigned char *)min));
        else
            fprintf(outf, "      null  ");
        if (max)
            fprintf(outf, (f ? fmt : "%10hhu  "), *((unsigned char *)max));
        else
            fprintf(outf, "      null  ");
        if (avg)
            fprintf(outf, "%10.2f  ", *avg);
        else
            fprintf(outf, "      null  ");
        if (std_dev)
            fprintf(outf, "%10.2f  ", *std_dev);
        else
            fprintf(outf, "      null  ");
        break;
    case adios_byte:
        if (min)
            fprintf(outf, (f ? fmt : "%10hhd  "), *((char *)min));
        else
            fprintf(outf, "      null  ");
        if (max)
            fprintf(outf, (f ? fmt : "%10hhd  "), *((char *)max));
        else
            fprintf(outf, "      null  ");
        if (avg)
            fprintf(outf, "%10.2f  ", *avg);
        else
            fprintf(outf, "      null  ");
        if (std_dev)
            fprintf(outf, "%10.2f  ", *std_dev);
        else
            fprintf(outf, "      null  ");
        break;
    case adios_string:
        break;

    case adios_unsigned_short:
        if (min)
            fprintf(outf, (f ? fmt : "%10hu  "), (*(unsigned short *)min));
        else
            fprintf(outf, "      null  ");
        if (max)
            fprintf(outf, (f ? fmt : "%10hu  "), (*(unsigned short *)max));
        else
            fprintf(outf, "      null  ");
        if (avg)
            fprintf(outf, "%10.2f  ", *avg);
        else
            fprintf(outf, "      null  ");
        if (std_dev)
            fprintf(outf, "%10.2f  ", *std_dev);
        else
            fprintf(outf, "      null  ");
        break;
    case adios_short:
        if (min)
            fprintf(outf, (f ? fmt : "%10hd  "), (*(short *)min));
        else
            fprintf(outf, "      null  ");
        if (max)
            fprintf(outf, (f ? fmt : "%10hd  "), (*(short *)max));
        else
            fprintf(outf, "      null  ");
        if (avg)
            fprintf(outf, "%10.2f  ", *avg);
        else
            fprintf(outf, "      null  ");
        if (std_dev)
            fprintf(outf, "%10.2f  ", *std_dev);
        else
            fprintf(outf, "      null  ");
        break;

    case adios_unsigned_integer:
        if (min)
            fprintf(outf, (f ? fmt : "%10u  "), (*(unsigned int *)min));
        else
            fprintf(outf, "      null  ");
        if (max)
            fprintf(outf, (f ? fmt : "%10u  "), (*(unsigned int *)max));
        else
            fprintf(outf, "      null  ");
        if (avg)
            fprintf(outf, "%10.2f  ", *avg);
        else
            fprintf(outf, "      null  ");
        if (std_dev)
            fprintf(outf, "%10.2f  ", *std_dev);
        else
            fprintf(outf, "      null  ");
        break;
    case adios_integer:
        if (min)
            fprintf(outf, (f ? fmt : "%10d  "), (*(int *)min));
        else
            fprintf(outf, "      null  ");
        if (max)
            fprintf(outf, (f ? fmt : "%10d  "), (*(int *)max));
        else
            fprintf(outf, "      null  ");
        if (avg)
            fprintf(outf, "%10.2f  ", *avg);
        else
            fprintf(outf, "      null  ");
        if (std_dev)
            fprintf(outf, "%10.2f  ", *std_dev);
        else
            fprintf(outf, "      null  ");
        break;

    case adios_unsigned_long:
        if (min)
            fprintf(outf, (f ? fmt : "%10llu  "), (*(unsigned long long *)min));
        else
            fprintf(outf, "      null  ");
        if (max)
            fprintf(outf, (f ? fmt : "%10llu  "), (*(unsigned long long *)max));
        else
            fprintf(outf, "      null  ");
        if (avg)
            fprintf(outf, "%10.2f  ", *avg);
        else
            fprintf(outf, "      null  ");
        if (std_dev)
            fprintf(outf, "%10.2f  ", *std_dev);
        else
            fprintf(outf, "      null  ");
        break;
    case adios_long:
        if (min)
            fprintf(outf, (f ? fmt : "%10lld  "), (*(long long *)min));
        else
            fprintf(outf, "      null  ");
        if (max)
            fprintf(outf, (f ? fmt : "%10lld  "), (*(long long *)max));
        else
            fprintf(outf, "      null  ");
        if (avg)
            fprintf(outf, "%10.2f  ", *avg);
        else
            fprintf(outf, "      null  ");
        if (std_dev)
            fprintf(outf, "%10.2f  ", *std_dev);
        else
            fprintf(outf, "      null  ");
        break;

    case adios_real:
        if (min)
            fprintf(outf, (f ? fmt : "%10.2g  "), (*(float *)min));
        else
            fprintf(outf, "      null  ");
        if (max)
            fprintf(outf, (f ? fmt : "%10.2g  "), (*(float *)max));
        else
            fprintf(outf, "      null  ");
        if (avg)
            fprintf(outf, "%10.2g  ", *avg);
        else
            fprintf(outf, "      null  ");
        if (std_dev)
            fprintf(outf, "%10.2g  ", *std_dev);
        else
            fprintf(outf, "      null  ");
        break;
    case adios_double:
        if (min)
            fprintf(outf, (f ? fmt : "%10.2g  "), (*(double *)min));
        else
            fprintf(outf, "      null  ");
        if (max)
            fprintf(outf, (f ? fmt : "%10.2g  "), (*(double *)max));
        else
            fprintf(outf, "      null  ");
        if (avg)
            fprintf(outf, "%10.2g  ", *avg);
        else
            fprintf(outf, "      null  ");
        if (std_dev)
            fprintf(outf, "%10.2g  ", *std_dev);
        else
            fprintf(outf, "      null  ");
        break;

    case adios_long_double:
        fprintf(outf, "????????");
        break;

    // TO DO
    /*
       case adios_complex:
       fprintf(outf,(f ? format : "(%g,i%g) "), ((float *) data)[2*item],
       ((float *) data)[2*item+1]);
       break;

       case adios_double_complex:
       fprintf(outf,(f ? format : "(%g,i%g)" ), ((double *) data)[2*item],
       ((double *) data)[2*item+1]);
       break;
     */
    default:
        break;
    } // end switch
    return 0;
}

int print_data(const void *data, int item, enum ADIOS_DATATYPES adiosvartype,
               bool allowformat)
{
    bool f = format.size() && allowformat;
    const char *fmt = format.c_str();
    if (data == NULL)
    {
        fprintf(outf, "null ");
        return 0;
    }
    // print next data item
    switch (adiosvartype)
    {
    case adios_unsigned_byte:
        fprintf(outf, (f ? fmt : "%hhu"), ((unsigned char *)data)[item]);
        break;
    case adios_byte:
        fprintf(outf, (f ? fmt : "%hhd"), ((signed char *)data)[item]);
        break;

    case adios_string:
        fprintf(outf, (f ? fmt : "\"%s\""), ((char *)data) + item);
        break;
    case adios_string_array:
        // we expect one elemet of the array here
        fprintf(outf, (f ? fmt : "\"%s\""), *((char **)data + item));
        break;

    case adios_unsigned_short:
        fprintf(outf, (f ? fmt : "%hu"), ((unsigned short *)data)[item]);
        break;
    case adios_short:
        fprintf(outf, (f ? fmt : "%hd"), ((signed short *)data)[item]);
        break;

    case adios_unsigned_integer:
        fprintf(outf, (f ? fmt : "%u"), ((unsigned int *)data)[item]);
        break;
    case adios_integer:
        fprintf(outf, (f ? fmt : "%d"), ((signed int *)data)[item]);
        break;

    case adios_unsigned_long:
        fprintf(outf, (f ? fmt : "%llu"), ((unsigned long long *)data)[item]);
        break;
    case adios_long:
        fprintf(outf, (f ? fmt : "%lld"), ((signed long long *)data)[item]);
        break;

    case adios_real:
        fprintf(outf, (f ? fmt : "%g"), ((float *)data)[item]);
        break;

    case adios_double:
        fprintf(outf, (f ? fmt : "%g"), ((double *)data)[item]);
        break;

    case adios_long_double:
        fprintf(outf, (f ? fmt : "%Lg"), ((long double *)data)[item]);
        // fprintf(outf,(f ? fmt : "????????"));
        break;

    case adios_complex:
        fprintf(outf, (f ? fmt : "(%g,i%g)"), ((float *)data)[2 * item],
                ((float *)data)[2 * item + 1]);
        break;

    case adios_double_complex:
        fprintf(outf, (f ? fmt : "(%g,i%g)"), ((double *)data)[2 * item],
                ((double *)data)[2 * item + 1]);
        break;

    default:
        break;
    } // end switch
    return 0;
}

int print_dataset(const void *data, const std::string vartype, uint64_t *s,
                  uint64_t *c, int tdims, int *ndigits)
{
    int i, item, steps;
    char idxstr[128], buf[16];
    uint64_t ids[MAX_DIMS]; // current indices
    bool roll;
    enum ADIOS_DATATYPES adiosvartype = type_to_enum(vartype);

    // init current indices
    steps = 1;
    for (i = 0; i < tdims; i++)
    {
        ids[i] = s[i];
        steps *= c[i];
    }

    item = 0; // index to *data
    // loop through each data item and print value
    while (item < steps)
    {

        // print indices if needed into idxstr;
        idxstr[0] = '\0'; // empty idx string
        if (nextcol == 0)
        {
            if (!noindex && tdims > 0)
            {
                sprintf(idxstr, "    (%*" PRIu64, ndigits[0], ids[0]);
                for (i = 1; i < tdims; i++)
                {
                    sprintf(buf, ",%*" PRIu64, ndigits[i], ids[i]);
                    strcat(idxstr, buf);
                }
                strcat(idxstr, ")    ");
            }
        }

        // print item
        fprintf(outf, "%s", idxstr);
        if (printByteAsChar &&
            (adiosvartype == adios_byte || adiosvartype == adios_unsigned_byte))
        {
            /* special case: k-D byte array printed as (k-1)D array of
             * strings
             */
            if (tdims == 0)
            {
                print_data_as_string(data, steps, adiosvartype);
            }
            else
            {
                print_data_as_string(
                    (char *)data + item, c[tdims - 1],
                    adiosvartype);        // print data of last dim as string
                item += c[tdims - 1] - 1; // will be ++-ed once below
                ids[tdims - 1] =
                    s[tdims - 1] + c[tdims - 1] - 1; // will be rolled below
            }
            nextcol = ncols - 1; // force new line, will be ++-ed once below
        }
        else
        {
            print_data(data, item, adiosvartype, true);
        }

        // increment/reset column index
        nextcol++;
        if (nextcol == ncols)
        {
            fprintf(outf, "\n");
            nextcol = 0;
        }
        else
        {
            fprintf(outf, " ");
        }

        // increment indices
        item++;
        roll = true;
        for (i = tdims - 1; i >= 0; i--)
        {
            if (roll)
            {
                if (ids[i] == s[i] + c[i] - 1)
                {
                    // last index in this dimension, roll upward
                    ids[i] = s[i];
                }
                else
                {
                    ids[i]++;
                    roll = false;
                }
            }
        }
    }
    return 0;
}

void print_endline(void)
{
    if (nextcol != 0)
        fprintf(outf, "\n");
    nextcol = 0;
}

template <class T>
void print_decomp(core::Engine *fp, core::IO *io, core::Variable<T> *variable)
{
    /* Print block info */
    // int blockid = 0;
    size_t nsteps = variable->GetAvailableStepsCount();
    size_t ndim = variable->m_Shape.size();
    // enum ADIOS_DATATYPES vartype = type_to_enum(variable->m_Type);
    // size_t nblocks = 1; /* FIXME: we need the number of blocks here */
    int ndigits_nsteps = ndigits(nsteps - 1);
    if (ndim == 0)
    {
        // scalars
        for (size_t i = 0; i < nsteps; i++)
        {
            fprintf(outf, "        step %*zu: ", ndigits_nsteps, i);
#if 0
            fprintf(outf, "%d instances available\n", vi->nblocks[i]);
            if (dump && vi->statistics && vi->statistics->blocks)
            {
                fprintf(outf, "               ");
                if (vi->statistics->blocks->mins)
                {
                    int col = 0;
                    for (int j = 0; j < vi->nblocks[i]; j++)
                    {
                        if (vartype == adios_complex ||
                            vartype == adios_double_complex)
                        {
                            print_data(vi->statistics->blocks->mins[blockid], 0,
                                       adios_double_complex, true);
                        }
                        else
                        {
                            print_data(vi->statistics->blocks->mins[blockid], 0,
                                       vartype, true);
                        }
                        ++col;
                        if (j < vi->nblocks[i] - 1)
                        {
                            if (col < ncols)
                            {
                                fprintf(outf, " ");
                            }
                            else
                            {
                                fprintf(outf, "\n               ");
                                col = 0;
                            }
                        }
                        ++blockid;
                    }
                    fprintf(outf, "\n");
                }
            }
#else
            fprintf(outf, "\n");
#endif
        }
        return;
    }
    else
    {
        // arrays
        /*
        int ndigits_nblocks;
        int ndigits_procid;
        int ndigits_time;
        int ndigits_dims[32];
        for (size_t k = 0; k < ndim; k++)
        {
            // get digit lengths for each dimension
            ndigits_dims[k] = ndigits(variable->m_Shape[k] - 1);
        }
        */

        for (size_t i = 0; i < nsteps; i++)
        {
            fprintf(outf, "        step %*zu: ", ndigits_nsteps, i);
            fprintf(outf, "\n");
#if 0
            ndigits_nblocks = ndigits(vi->nblocks[i] - 1);
            ndigits_procid =
                ndigits(vi->blockinfo[blockid + vi->nblocks[i] - 1].process_id);
            ndigits_time =
                ndigits(vi->blockinfo[blockid + vi->nblocks[i] - 1].time_index);
            for (int j = 0; j < vi->nblocks[i]; j++)
            {
                if (verbose < 1)
                {
                    fprintf(outf, "          block %*d: [", ndigits_nblocks, j);
                }
                else
                {
                    fprintf(outf, "          block %*d proc %*u time %*u: [",
                            ndigits_nblocks, j, ndigits_procid,
                            vi->blockinfo[blockid].process_id, ndigits_time,
                            vi->blockinfo[blockid].time_index);
                }
                for (int k = 0; k < ndim; k++)
                {
                    if (vi->blockinfo[blockid].count[k])
                    {
                        fprintf(outf, "%*" PRIu64 ":%*" PRIu64, ndigits_dims[k],
                                vi->blockinfo[blockid].start[k],
                                ndigits_dims[k],
                                vi->blockinfo[blockid].start[k] +
                                    vi->blockinfo[blockid].count[k] - 1);
                    }
                    else
                    {
                        fprintf(outf, "%-*s", 2 * ndigits_dims[k] + 1, "null");
                    }
                    if (k < ndim - 1)
                        fprintf(outf, ", ");
                }
                fprintf(outf, "]");

                /* Print per-block statistics if available */
                if (longopt && vi->statistics->blocks)
                {
                    fprintf(outf, " = ");
                    if (vi->statistics->blocks->mins)
                    {
                        if (vartype == adios_complex ||
                            vartype == adios_double_complex)
                        {
                            print_data(vi->statistics->blocks->mins[blockid], 0,
                                       adios_double_complex, false);
                        }
                        else
                        {
                            print_data(vi->statistics->blocks->mins[blockid], 0,
                                       vartype, false);
                        }
                    }
                    else
                    {
                        fprintf(outf, "N/A ");
                    }

                    fprintf(outf, " / ");
                    if (vi->statistics->blocks->maxs)
                    {
                        if (vartype == adios_complex ||
                            vartype == adios_double_complex)
                        {
                            print_data(vi->statistics->blocks->maxs[blockid], 0,
                                       adios_double_complex, false);
                        }
                        else
                        {
                            print_data(vi->statistics->blocks->maxs[blockid], 0,
                                       vartype, false);
                        }
                    }
                    else
                    {
                        fprintf(outf, "N/A ");
                    }

                    fprintf(outf, "/ ");
                    if (vi->statistics->blocks->avgs)
                    {
                        if (vartype == adios_complex ||
                            vartype == adios_double_complex)
                        {
                            print_data(vi->statistics->blocks->avgs[blockid], 0,
                                       adios_double_complex, false);
                        }
                        else
                        {
                            print_data(vi->statistics->blocks->avgs[blockid], 0,
                                       adios_double, false);
                        }
                    }
                    else
                    {
                        fprintf(outf, "N/A ");
                    }

                    fprintf(outf, "/ ");
                    if (vi->statistics->blocks->avgs)
                    {
                        if (vartype == adios_complex ||
                            vartype == adios_double_complex)
                        {
                            print_data(
                                vi->statistics->blocks->std_devs[blockid], 0,
                                adios_double_complex, false);
                        }
                        else
                        {
                            print_data(
                                vi->statistics->blocks->std_devs[blockid], 0,
                                adios_double, false);
                        }
                    }
                    else
                    {
                        fprintf(outf, "N/A ");
                    }
                }
                fprintf(outf, "\n");
                if (dump)
                {
                    readVarBlock(fp, io, variable, blockid);
                }
                blockid++;
            }
#endif
        }
    }
}

// parse a string "0, 3; 027" into an integer array
// of [0,3,27]
// exits if parsing failes
void parseDimSpec(const std::string &str, int64_t *dims)
{
    if (str.empty())
        return;

    char *token;
    char *s; // copy of s; strtok modifies the string
    int i = 0;

    s = mystrndup(str.c_str(), 1024);
    // token = strtok_r(s, " ,;x\t\n", &saveptr);
    token = strtok(s, " ,;x\t\n");
    while (token != NULL && i < MAX_DIMS)
    {
        // printf("\t|%s|", token);
        errno = 0;
        dims[i] = (int64_t)strtoll(token, (char **)NULL, 0);
        if (errno)
        {
            fprintf(stderr, "Error: could not convert field into a value: "
                            "%s from \"%s\"\n",
                    token, str.c_str());
            exit(200);
        }

        // get next item
        // token = strtok_r(NULL, " ,;x\t\n", &saveptr);
        token = strtok(NULL, " ,;x\t\n");
        i++;
    }
    // if (i>0) printf("\n");

    if (i > ndimsspecified)
        ndimsspecified = i;

    // check if number of dims specified is larger than we can handle
    if (token != NULL)
    {
        fprintf(stderr, "Error: More dimensions specified in \"%s\" than we "
                        "can handle (%d)\n",
                str.c_str(), MAX_DIMS);
        exit(200);
    }
    free(s);
}

int compile_regexp_masks(void)
{
#ifdef USE_C_REGEX
    int errcode;
    char buf[256];
    for (int i = 0; i < nmasks; i++)
    {
        errcode = regcomp(&(varregex[i]), varmask[i], REG_EXTENDED);
        if (errcode)
        {
            regerror(errcode, &(varregex[i]), buf, sizeof(buf));
            fprintf(stderr, "Error: \"%s\" is an invalid extended regular "
                            "expression: %s\n",
                    varmask[i], buf);
            return 2;
        }
    }
#else
    varregex.reserve(nmasks);
    for (int i = 0; i < nmasks; i++)
    {
        try
        {
            varregex.push_back(std::regex(varmask[i]));
        }
        catch (std::regex_error &e)
        {
            fprintf(stderr, "Error: \"%s\" is an invalid extended regular "
                            "expression. C++ regex error code: %d\n",
                    varmask[i],
                    e.code() == std::regex_constants::error_badrepeat);
            return 2;
        }
    }
#endif
    return 0;
}

char *mystrndup(const char *s, size_t n)
{
    char *t = nullptr;
    if (n > 0)
    {
        size_t slen = strlen(s);
        size_t len = (slen <= n ? slen : n);
        t = (char *)malloc(len + 1);
        if (t)
        {
            memcpy(t, s, len);
            t[len] = '\0';
        }
    }
    return t;
}

// end of namespace
}
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int retval = adios2::utils::bplsMain(argc, argv);
    MPI_Finalize();
    return retval;
}
