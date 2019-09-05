/*
 * ADIOS is freely available under the terms of the BSD license described
 * in the COPYING file in the top level directory of this source distribution.
 *
 * Copyright (c) 2008 - 2009.  UT-BATTELLE, LLC. All rights reserved.
 */

/*
 * BPSPLIT: select steps and variables from a BP4 dataset
 *
 * Author: Norbert Podhorszki, pnorbert@ornl.gov
 *
 **/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "bpsplit.h"

#include <cinttypes>
#include <cstdio>
#include <string>
#include <vector>

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

#include <adios2sys/CommandLineArguments.hxx>
#include <adios2sys/SystemTools.hxx>

namespace adios2
{
namespace utils
{

// global variables
// Values from the arguments or defaults

// output files' starting path (can be extended with subdirs,
// names, indexes)
std::string outpath;
std::string inpath;
std::vector<std::string> varmask; // can have many -var masks (either shell
                                  // patterns or extended regular expressions)
int nmasks;                       // number of masks specified

// Flags from arguments or defaults
bool dump;       // dump data not just list info
bool use_regexp; // use varmasks as regular expressions
bool sortnames;  // sort names before listing
bool listattrs;  // do list attributes too
bool listmeshes; // do list meshes too
bool attrsonly;  // do list attributes only
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
        "usage: bpsplit [OPTIONS] file [mask1 mask2 ...]\n"
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
        "Typical use: bpsplit -lav <file>\n");
}

bool option_help_was_called = false;
int optioncb_help(const char *argument, const char *value, void *call_data)
{
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
    retry_args.push_back(new char[4]());

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
        else if (inpath.empty())
        {
            inpath = std::string(uargs[i], 4096);
            std::cout << "Set file argument: " << inpath << std::endl;
        }
        else if (outpath.empty())
        {
            outpath = std::string(uargs[i], 4096);
            std::cout << "Set file argument: " << outpath << std::endl;
        }
        else
        {
            varmask.push_back(std::string(uargs[i], 256));
            std::cout << "Set mask " << std::to_string(nmasks)
                      << " argument: " << varmask[nmasks] << std::endl;
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
int bpsplitMain(int argc, char *argv[])
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
                    "Print information about what bpsplit is doing");
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

    /* Check if we have the input file defined */
    if (inpath.empty())
    {
        fprintf(stderr, "Missing input file name\n");
        return 1;
    }

    /* Check if we have the output file defined */
    if (outpath.empty())
    {
        fprintf(stderr, "Missing output file name\n");
        return 1;
    }

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

    if (attrsonly)
        listattrs = true;

    if (verbose > 1)
        printSettings();

    /* Start working */
    retval = doSplit();

    /* Free allocated memories */
    for (int i = 0; i < nmasks; i++)
    {
#ifdef USE_C_REGEX
        regfree(&(varregex[i]));
#else
        varregex.clear();
#endif
    }
    return retval;
}

void init_globals()
{
    int i;
    // variables for arguments
    nmasks = 0;
    verbose = 0;
    ncols = 6; // by default when printing ascii, print "X Y", not X: Y1 Y2...
    dump = false;
    noindex = false;
    timestep = false;
    sortnames = false;
    listattrs = false;
    listmeshes = false;
    attrsonly = false;
    longopt = false;
    // timefrom             = 1;
    // timeto               = -1;
    use_regexp = false;
    plot = false;
    hidden_attrs = false;
    hidden_attrs_flag = 0;
    printByteAsChar = false;
    show_decomp = false;
}

void printSettings(void)
{
    int i;
    std::cout << "Settings :\n";
    std::cout << "  masks  : " << nmasks << ": ";
    for (i = 0; i < nmasks; i++)
        std::cout << varmask[i] << " ";
    std::cout << "\n";
    std::cout << "  file   : " << inpath << "\n";
    std::cout << "  output : " << outpath << "\n";

    if (longopt)
        std::cout
            << "      -l : show scalar values and min/max/avg of arrays\n";
    if (sortnames)
        std::cout << "      -r : sort names before listing\n";
    if (attrsonly)
        std::cout << "      -A : list attributes only\n";
    else if (listattrs)
        std::cout << "      -a : list attributes too\n";
    else if (listmeshes)
        std::cout << "      -m : list meshes too\n";
    if (dump)
        std::cout << "      -d : dump matching variables and attributes\n";
    if (use_regexp)
        std::cout << "      -e : handle masks as regular expressions\n";
    if (show_decomp)
        std::cout << "      -D : show decomposition of variables in the file\n";
    if (hidden_attrs)
    {
        std::cout << "         : show hidden attributes in the file\n";
    }
}

int nEntriesMatched = 0;

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

int doSplit()
{
    char init_params[128];
    int adios_verbose = 2;

    if (verbose > 1)
        printf("\nADIOS Open: read header info from %s\n", inpath);

    if (!adios2sys::SystemTools::FileExists(inpath))
    {
        fprintf(stderr, "\nError: input path %s does not exist\n", inpath);
        return 4;
    }

    // initialize BP reader
    if (verbose > 1)
        adios_verbose = 3; // print info lines
    if (verbose > 2)
        adios_verbose = 4; // print debug lines
    sprintf(init_params, "verbose=%d", adios_verbose);
    if (hidden_attrs)
        strcat(init_params, ";show_hidden_attrs");

    core::ADIOS adios(true, "C++");
    core::IO &io = adios.DeclareIO("bpsplit");
    core::Engine *fp = nullptr;
    std::vector<std::string> engineList = getEnginesList(inpath);
    for (auto &engineName : engineList)
    {
        if (verbose > 2)
            printf("Try %s engine to open the file...\n", engineName.c_str());
        io.SetEngine(engineName);
        try
        {
            fp = &io.Open(inpath, Mode::Read);
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
        // besides the last parameter, which is an array of strings for
        // holding
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
            if (fnmatch(varmask[i].c_str(), name + startpos, FNM_FILE_NAME) ==
                0)
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

template <class T>
size_t relative_to_absolute_step(core::Variable<T> *variable,
                                 const size_t relstep)
{
    const std::map<size_t, std::vector<size_t>> &indices =
        variable->m_AvailableStepBlockIndexOffsets;
    auto itStep = indices.begin();
    size_t absstep = itStep->first - 1;

    for (int step = 0; step < relstep; step++)
    {
        ++itStep;
        absstep = itStep->first - 1;
    }
    return absstep;
}

int compile_regexp_masks(void)
{
#ifdef USE_C_REGEX
    int errcode;
    char buf[256];
    for (int i = 0; i < varmask.size(); i++)
    {
        errcode = regcomp(&(varregex[i]), varmask[i], REG_EXTENDED);
        if (errcode)
        {
            regerror(errcode, &(varregex[i]), buf, sizeof(buf));
            fprintf(stderr,
                    "Error: \"%s\" is an invalid extended regular "
                    "expression: %s\n",
                    varmask[i], buf);
            return 2;
        }
    }
#else
    varregex.reserve(varmask.size());
    for (int i = 0; i < varmask.size(); i++)
    {
        try
        {
            varregex.push_back(std::regex(varmask[i]));
        }
        catch (std::regex_error &e)
        {
            fprintf(stderr,
                    "Error: \"%s\" is an invalid extended regular "
                    "expression. C++ regex error code: %d\n",
                    varmask[i],
                    e.code() == std::regex_constants::error_badrepeat);
            return 2;
        }
    }
#endif
    return 0;
}

// end of namespace
}
}

int main(int argc, char *argv[])
{
    int retval = 1;
    try
    {
        retval = adios2::utils::bpsplitMain(argc, argv);
    }
    catch (std::exception &e)
    {
        std::cout << "\nbpsplit caught an exception\n";
        std::cout << e.what() << std::endl;
    }
    return retval;
}
