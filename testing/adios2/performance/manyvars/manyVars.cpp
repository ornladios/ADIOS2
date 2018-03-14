/*
 * ADIOS is freely available under the terms of the BSD license described
 * in the COPYING file in the top level directory of this source distribution.
 *
 * Copyright (c) 2008 - 2009.  UT-BATTELLE, LLC. All rights reserved.
 */

/* ADIOS C test:
 *  Write a huge number of variables
 *  Then read them all and check if they are correct.
 *
 * How to run: mpirun -np <N> many_vars <nvars> <blocks per process> <steps>
 * Output: many_vars.bp
 *
 */

#include <vector>

#include <gtest/gtest.h>

#include "adios2_c.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct RunParams
{
    size_t nvars;
    size_t nblocks;
    size_t nsteps;
    RunParams(size_t nv, size_t nb, size_t ns)
    : nvars{nv}, nblocks{nb}, nsteps{ns} {};
};

/* This function is executed by INSTANTIATE_TEST_CASE_P
   before main() and MPI_Init()!!! */
std::vector<RunParams> CreateRunParams()
{
    std::vector<RunParams> params;
    // 1 variable
    params.push_back(RunParams(1, 1, 1));
    params.push_back(RunParams(1, 1, 2));
    params.push_back(RunParams(1, 2, 1));
    params.push_back(RunParams(1, 2, 2));
    // 2 variables
    params.push_back(RunParams(2, 1, 1));
    params.push_back(RunParams(2, 1, 2));
    params.push_back(RunParams(2, 2, 1));
    params.push_back(RunParams(2, 2, 2));

    // These pass
    params.push_back(RunParams(5, 14, 1));
    params.push_back(RunParams(8, 8, 1));

    // These fail
    // params.push_back(RunParams(5, 15, 1));
    // params.push_back(RunParams(8, 9, 1));

    return params;
}

#define log(...)                                                               \
    fprintf(stderr, "[rank=%3.3d, line %d]: ", rank, __LINE__);                \
    fprintf(stderr, __VA_ARGS__);                                              \
    fflush(stderr);
#define printE(...)                                                            \
    fprintf(stderr, "[rank=%3.3d, line %d]: ERROR: ", rank, __LINE__);         \
    fprintf(stderr, __VA_ARGS__);                                              \
    fflush(stderr);

#define VALUE(rank, step, block) (step * 10000 + 10 * rank + block)

#define CHECK_VARINFO(VARNAME, NDIM, NSTEPS)                                   \
    vi = adios2_inquire_variable(ioR, VARNAME);                                \
    if (vi == NULL)                                                            \
    {                                                                          \
        printE("No such variable: %s\n", VARNAME);                             \
        err = 101;                                                             \
        goto endread;                                                          \
    }                                                                          \
    if (adios2_variable_ndims(vi) != NDIM)                                     \
    {                                                                          \
        printE("Variable %s has %zu dimensions, but expected %u\n", VARNAME,   \
               adios2_variable_ndims(vi), NDIM);                               \
        err = 102;                                                             \
        goto endread;                                                          \
    }                                                                          \
    if (adios2_variable_available_steps_count(vi) != NSTEPS)                   \
    {                                                                          \
        printE("Variable %s has %zu steps, but expected %u\n", VARNAME,        \
               adios2_variable_available_steps_count(vi), NSTEPS);             \
        err = 103;                                                             \
        /*goto endread; */                                                     \
    }

#define CHECK_SCALAR(VARNAME, VAR, VALUE, STEP)                                \
    if (VAR != VALUE)                                                          \
    {                                                                          \
        printE(#VARNAME " step %d: wrote %d but read %d\n", STEP, VALUE, VAR); \
        err = 104;                                                             \
        /*goto endread;*/                                                      \
    }

#define CHECK_ARRAY(VARNAME, A, N, VALUE, STEP, BLOCK, i)                      \
    for (i = 0; i < N; i++)                                                    \
        if (A[i] != VALUE)                                                     \
        {                                                                      \
            printE("%s[%d] step %d block %d: wrote %d but read %d\n", VARNAME, \
                   i, STEP, BLOCK, VALUE, A[i]);                               \
            err = 104;                                                         \
            /*goto endread;*/                                                  \
            break;                                                             \
        }

MPI_Comm comm = MPI_COMM_WORLD;
int rank;
int numprocs;

class TestManyVars : public ::testing::TestWithParam<RunParams>
{
public:
    TestManyVars() = default;
    ~TestManyVars() = default;

    int NVARS = 1;
    int NBLOCKS = 1;
    int NSTEPS = 1;
    int REDEFINE =
        0; // 1: delete and redefine variable definitions at each step to
           // test adios_delete_vardefs()
    char *FILENAME = nullptr;

    /* Variables to write */
    int *a2 = nullptr;

    static const size_t ldim1 = 5;
    static const size_t ldim2 = 5;
    size_t gdim1, gdim2;
    size_t offs1, offs2;

    /* ADIOS2 variables to be accessed from multiple functions */
    adios2_IO *ioW, *ioR;
    adios2_Engine *engineW;
    adios2_Variable **varW; // array to hold all variable definitions

    /* Variables to read */
    int *r2;
    char **varnames;

    void alloc_vars()
    {
        int n, i;

        n = ldim1 * ldim2;
        a2 = (int *)malloc(n * sizeof(int));
        r2 = (int *)malloc(n * sizeof(int));
        varW = (adios2_Variable **)malloc(NVARS * sizeof(adios2_Variable *));

        varnames = (char **)malloc(NVARS * sizeof(char *));
        for (i = 0; i < NVARS; i++)
        {
            varnames[i] = (char *)malloc(16);
        }

        /* make varnames like v001,v002,.. */
        int digit = 1, d = 10;
        while (NVARS / d > 0)
        {
            d *= 10;
            digit++;
        }

        char fmt[16];
        sprintf(fmt, "v%%%d.%dd", digit, digit);
        for (i = 0; i < NVARS; i++)
        {
            sprintf(varnames[i], fmt, i);
        }
    }

    void set_gdim()
    {
        gdim1 = numprocs * ldim1;
        gdim2 = NBLOCKS * ldim2;
    }

    void set_offsets(int block)
    {
        offs1 = rank * ldim1;
        offs2 = block * ldim2;
    }

    void set_vars(int step, int block)
    {
        int n, i;
        int v = VALUE(rank, step, block);

        set_offsets(block);

        n = ldim1 * ldim2;
        for (i = 0; i < n; i++)
            a2[i] = v;
    }

    void fini_vars()
    {
        int i;
        free(a2);
        free(r2);
        free(varW);
        for (i = 0; i < NVARS; i++)
        {
            free(varnames[i]);
        }
        free(varnames);
    }

    void Usage()
    {
        printf("Usage: many_vars <nvars> <nblocks> <nsteps> [redef]\n"
               "    <nvars>:   Number of variables to generate\n"
               "    <nblocks>: Number of blocks per process to write\n"
               "    <nsteps>:  Number of write cycles (to same file)\n"
               "    [redef]:   delete and redefine variables at every step\n");
    }

    int Test(RunParams p, bool redefineVars)
    {
        int err, i;

        NVARS = p.nvars;
        NBLOCKS = p.nblocks;
        NSTEPS = p.nsteps;
        REDEFINE = redefineVars;
        std::string fn = "manyVars." + std::to_string(NVARS) + "_" +
                         std::to_string(NBLOCKS) + "_" +
                         std::to_string(NSTEPS) + "_" +
                         (REDEFINE ? "redefine" : "") + ".bp";
        FILENAME = const_cast<char *>(fn.c_str());

        alloc_vars();
        adios2_ADIOS *adiosH =
            adios2_init(MPI_COMM_WORLD, adios2_debug_mode_on);
        ioW = adios2_declare_io(adiosH, "multiblockwrite"); // group for writing
        ioR = adios2_declare_io(adiosH, "multiblockread");  // group for reading
        set_gdim();

        if (rank == 0)
        {
            log("Test %d Variables, %d Blocks, %d Steps\n", NVARS, NBLOCKS,
                NSTEPS);
        }

        engineW = adios2_open(ioW, FILENAME, adios2_mode_write);

        err = 0;
        for (i = 0; i < NSTEPS; i++)
        {
            if (!err)
            {
                if (i == 0 || REDEFINE)
                {
                    printf("-- Define variables.\n");
                    define_vars();
                }

                err = write_file(i);

                if (REDEFINE)
                {
                    printf("-- Delete variable definitions.\n");
                    adios2_remove_all_variables(ioW);
                    adios2_remove_all_attributes(ioW);
                }
            }
        }
        adios2_close(engineW);

        if (!err)
            err = read_file();

        adios2_finalize(adiosH);
        fini_vars();
        return err;
    }

    void define_vars()
    {
        int i, block;

        size_t shape[2] = {gdim1, gdim2};
        size_t count[2] = {ldim1, ldim2};
        size_t start[2] = {0, 0};

        /* One variable definition for many blocks.
         * Offsets will change at writing for each block. */
        for (i = 0; i < NVARS; i++)
        {
            varW[i] = adios2_define_variable(ioW, varnames[i], adios2_type_int,
                                             2, shape, start, count,
                                             adios2_constant_dims_false, a2);
        }
    }

    int write_file(int step)
    {
        int64_t fh;
        uint64_t groupsize = 0, totalsize;
        int block, v, i;
        double tb, te;
        size_t count[2] = {ldim1, ldim2};

        log("Write step %d to %s\n", step, FILENAME);
        tb = MPI_Wtime();

        adios2_begin_step(engineW, adios2_step_mode_append, 0.0);
        for (block = 0; block < NBLOCKS; block++)
        {
            v = VALUE(rank, step, block);
            log("  Write block %d, value %d to %s\n", block, v, FILENAME);
            set_vars(step, block);
            size_t start[2] = {offs1, offs2};
            for (i = 0; i < NVARS; i++)
            {
                adios2_set_selection(varW[i], 2, start, count);
                adios2_put_sync(engineW, varW[i], a2);
            }
        }
        adios2_perform_puts(engineW);
        adios2_end_step(engineW);

        te = MPI_Wtime();
        if (rank == 0)
        {
            log("  Write time for step %d was %6.3lf seconds\n", step, te - tb);
        }
        MPI_Barrier(comm);
        return 0;
    }

    void reset_readvars()
    {
        int n;

        n = ldim1 * ldim2;
        memset(r2, -1, n * sizeof(int));
    }

    int read_file()
    {
        adios2_Variable *vi;
        int err = 0, v, n;
        int block, step, i; // loop variables
        int iMacro;         // loop variable in macros
        double tb, te, tsched;
        double tsb, ts; // time for just scheduling for one step/block

        uint64_t start[2] = {offs1, offs2};
        uint64_t count[2] = {ldim1, ldim2};
        uint64_t ndim;

        reset_readvars();

        log("Read and check data in %s\n", FILENAME);
        adios2_Engine *engineR = adios2_open(ioR, FILENAME, adios2_mode_read);
        if (engineR == NULL)
        {
            printE("Error at opening file %s for reading\n", FILENAME);
            return 1;
        }

        adios2_step_status status =
            adios2_begin_step(engineR, adios2_step_mode_next_available, 0.0);

        log("  Check variable definitions... %s\n", FILENAME);
        tb = MPI_Wtime();
        for (i = 0; i < NVARS; i++)
        {
            CHECK_VARINFO(varnames[i], 2, NSTEPS)
        }
        MPI_Barrier(comm);
        te = MPI_Wtime();
        if (rank == 0)
        {
            log("  Time to check all vars' info: %6.3lf seconds\n", te - tb);
        }

        log("  Check variable content...\n");
        for (step = 0; step < NSTEPS; step++)
        {
            tb = MPI_Wtime();
            ts = 0;
            if (step > 0)
            {
                status = adios2_begin_step(
                    engineR, adios2_step_mode_next_available, 0.0);
            }
            for (block = 0; block < NBLOCKS; block++)
            {

                if (status == adios2_step_status_ok)
                {
                    v = VALUE(rank, step, block);
                    set_offsets(block);
                    start[0] = offs1;
                    start[1] = offs2;

                    log("    Step %d block %d: value=%d\n", step, block, v);

                    for (i = 0; i < NVARS; i++)
                    {
                        tsb = MPI_Wtime();
                        adios2_Variable *varH =
                            adios2_inquire_variable(ioR, varnames[i]);
                        adios2_set_selection(varH, 2, start, count);
                        adios2_get_sync(engineR, varH, r2);
                        ts += MPI_Wtime() - tsb;
                        CHECK_ARRAY(varnames[i], r2, ldim1 * ldim2, v, step,
                                    block, iMacro)
                    }
                }
                else
                {
                    printf("-- ERROR: Could not get Step %d, status = %d\n", i,
                           status);
                }
            }
            adios2_end_step(engineR);
            MPI_Barrier(comm);
            te = MPI_Wtime();
            if (rank == 0)
            {
                log("  Read time for step %d was %6.3lfs\n", step, ts);
            }
        }

    endread:

        adios2_close(engineR);
        MPI_Barrier(comm);
        return err;
    }
};

TEST_P(TestManyVars, DontRedefineVars)
{
    RunParams p = GetParam();
    int err = Test(p, false);
    ASSERT_EQ(err, 0);
}

INSTANTIATE_TEST_CASE_P(NxM, TestManyVars,
                        ::testing::ValuesIn(CreateRunParams()));

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    ::testing::InitGoogleTest(&argc, argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    int result = RUN_ALL_TESTS();

    MPI_Finalize();
    return result;
}
