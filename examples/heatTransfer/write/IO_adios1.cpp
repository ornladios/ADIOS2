/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO_ADIOS1.cpp
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */

#include <iomanip>
#include <iostream>
#include <string>

#include "adios2/IO.h"
#include "adios2/adios.h"

static int64_t group;
static int rank_saved;

IO::IO(const Settings &s, MPI_Comm comm)
{
    rank_saved = s.rank;
    m_outputfilename = s.outputfile + ".bp";
    adios_init_noxml(comm);
    adios_declare_group(&group, "heat", "", adios_stat_default);
    adios_select_method(group, "MPI", "", "");

    adios_define_var(group, "gndx", "", adios_integer, "", "", "");
    adios_define_var(group, "gndy", "", adios_integer, "", "", "");

    std::string ldims(std::to_string(s.ndx) + "," + std::to_string(s.ndy));
    std::string gdims(std::to_string(s.gndx) + "," + std::to_string(s.gndy));
    std::string offs(std::to_string(s.offsx) + "," + std::to_string(s.offsy));
    uint64_t T_id;
    T_id = adios_define_var(group, "T", "", adios_double, ldims.c_str(),
                            gdims.c_str(), offs.c_str());

    adios_set_transform(T_id, "none");
    // adios_set_transform( T_id, "zfp:accuracy=0.001");
}

IO::~IO() { adios_finalize(rank_saved); }

void IO::write(int step, const HeatTransfer &ht, const Settings &s,
               MPI_Comm comm)
{
    char mode[2] = "w";
    if (step > 0)
    {
        mode[0] = 'a';
    }

    // for time measurements, let's synchronize the processes
    MPI_Barrier(comm);
    double time_start = MPI_Wtime();

    int64_t f;
    adios_open(&f, "heat", m_outputfilename.c_str(), mode, comm);
    adios_write(f, "gndx", &s.gndx);
    adios_write(f, "gndy", &s.gndy);
    adios_write(f, "T", ht.data_noghost().data());
    adios_close(f);

    MPI_Barrier(comm);
    double total_time = MPI_Wtime() - time_start;
    uint64_t adios_totalsize =
        2 * sizeof(int) + 2 * s.ndx * s.ndy * sizeof(double);
    uint64_t sizeMB =
        adios_totalsize * s.nproc / 1024 / 1024 / 1024; // size in MB
    uint64_t mbs = sizeMB / total_time;
    if (s.rank == 0)
        std::cout << "Step " << step << ": " << m_outputfilename << " "
                  << sizeMB << " " << total_time << "" << mbs << std::endl;
}
