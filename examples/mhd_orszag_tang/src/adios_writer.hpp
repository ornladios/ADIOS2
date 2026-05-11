/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <adios2.h>

#include "mhd2d.hpp"

#ifdef OT_USE_MPI
#include <mpi.h>
#endif

namespace ot
{

class AdiosWriter
{
public:
#ifdef OT_USE_MPI
    AdiosWriter(const SimulationParams &params, int global_nx, int global_ny, int local_ny,
                int y_offset, int rank, MPI_Comm comm);
#else
    AdiosWriter(const SimulationParams &params, int global_nx, int global_ny, int local_ny,
                int y_offset, int rank);
#endif

    ~AdiosWriter();

    void write(int step, double time, const OutputFields &fields,
               const ScalarDiagnostics &diagnostics);
    void close();

private:
    int rank_ = 0;
    bool closed_ = false;

    adios2::ADIOS adios_;
    adios2::IO io_;
    adios2::Engine engine_;

    adios2::Variable<double> rho_;
    adios2::Variable<double> pressure_;
    adios2::Variable<double> vx_;
    adios2::Variable<double> vy_;
    adios2::Variable<double> vz_;
    adios2::Variable<double> bx_;
    adios2::Variable<double> by_;
    adios2::Variable<double> bz_;
    adios2::Variable<double> speed_;
    adios2::Variable<double> current_z_;
    adios2::Variable<double> psi_;
    adios2::Variable<double> mx_;
    adios2::Variable<double> my_;
    adios2::Variable<double> mz_;
    adios2::Variable<double> E_;

    adios2::Variable<int> step_;
    adios2::Variable<double> time_;
    adios2::Variable<double> mass_;
    adios2::Variable<double> kinetic_energy_;
    adios2::Variable<double> magnetic_energy_;
    adios2::Variable<double> internal_energy_;
    adios2::Variable<double> total_energy_;
    adios2::Variable<double> mean_pressure_;
    adios2::Variable<double> max_speed_;
    adios2::Variable<double> current_abs_max_;
    adios2::Variable<double> current_rms_;
    adios2::Variable<double> divb_abs_max_;
    adios2::Variable<double> divb_l2_;
};

} // namespace ot
