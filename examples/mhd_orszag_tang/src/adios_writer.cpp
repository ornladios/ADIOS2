/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adios_writer.hpp"

#include <stdexcept>

namespace ot
{

namespace
{

std::string with_var_prefix(const SimulationParams &params, const char *logical_name)
{
    return params.prepend_var_names + logical_name;
}

} // namespace

#ifdef OT_USE_MPI
AdiosWriter::AdiosWriter(const SimulationParams &params, int global_nx, int global_ny, int local_ny,
                         int y_offset, int rank, MPI_Comm comm)
: rank_(rank), adios_(comm), io_(adios_.DeclareIO("ot_io"))
{
#else
AdiosWriter::AdiosWriter(const SimulationParams &params, int global_nx, int global_ny, int local_ny,
                         int y_offset, int rank)
: rank_(rank), adios_(), io_(adios_.DeclareIO("ot_io"))
{
#endif
    io_.SetEngine(params.adios_engine);

    const adios2::Dims shape = {static_cast<std::size_t>(global_ny),
                                static_cast<std::size_t>(global_nx)};
    const adios2::Dims start = {static_cast<std::size_t>(y_offset), 0};
    const adios2::Dims count = {static_cast<std::size_t>(local_ny),
                                static_cast<std::size_t>(global_nx)};

    rho_ = io_.DefineVariable<double>(with_var_prefix(params, "rho"), shape, start, count,
                                      adios2::ConstantDims);
    pressure_ = io_.DefineVariable<double>(with_var_prefix(params, "pressure"), shape, start, count,
                                           adios2::ConstantDims);
    vx_ = io_.DefineVariable<double>(with_var_prefix(params, "vx"), shape, start, count,
                                     adios2::ConstantDims);
    vy_ = io_.DefineVariable<double>(with_var_prefix(params, "vy"), shape, start, count,
                                     adios2::ConstantDims);
    vz_ = io_.DefineVariable<double>(with_var_prefix(params, "vz"), shape, start, count,
                                     adios2::ConstantDims);
    bx_ = io_.DefineVariable<double>(with_var_prefix(params, "bx"), shape, start, count,
                                     adios2::ConstantDims);
    by_ = io_.DefineVariable<double>(with_var_prefix(params, "by"), shape, start, count,
                                     adios2::ConstantDims);
    bz_ = io_.DefineVariable<double>(with_var_prefix(params, "bz"), shape, start, count,
                                     adios2::ConstantDims);
    speed_ = io_.DefineVariable<double>(with_var_prefix(params, "speed"), shape, start, count,
                                        adios2::ConstantDims);
    current_z_ = io_.DefineVariable<double>(with_var_prefix(params, "current_z"), shape, start,
                                            count, adios2::ConstantDims);
    if (params.output_psi)
    {
        psi_ = io_.DefineVariable<double>(with_var_prefix(params, "psi"), shape, start, count,
                                          adios2::ConstantDims);
    }
    if (params.output_m)
    {
        mx_ = io_.DefineVariable<double>(with_var_prefix(params, "mx"), shape, start, count,
                                         adios2::ConstantDims);
        my_ = io_.DefineVariable<double>(with_var_prefix(params, "my"), shape, start, count,
                                         adios2::ConstantDims);
        mz_ = io_.DefineVariable<double>(with_var_prefix(params, "mz"), shape, start, count,
                                         adios2::ConstantDims);
    }
    if (params.output_E)
    {
        E_ = io_.DefineVariable<double>(with_var_prefix(params, "E"), shape, start, count,
                                        adios2::ConstantDims);
    }

    step_ = io_.DefineVariable<int>(with_var_prefix(params, "step"));
    time_ = io_.DefineVariable<double>(with_var_prefix(params, "time"));
    mass_ = io_.DefineVariable<double>(with_var_prefix(params, "mass"));
    kinetic_energy_ = io_.DefineVariable<double>(with_var_prefix(params, "kinetic_energy"));
    magnetic_energy_ = io_.DefineVariable<double>(with_var_prefix(params, "magnetic_energy"));
    internal_energy_ = io_.DefineVariable<double>(with_var_prefix(params, "internal_energy"));
    total_energy_ = io_.DefineVariable<double>(with_var_prefix(params, "total_energy"));
    mean_pressure_ = io_.DefineVariable<double>(with_var_prefix(params, "mean_pressure"));
    max_speed_ = io_.DefineVariable<double>(with_var_prefix(params, "max_speed"));
    current_abs_max_ = io_.DefineVariable<double>(with_var_prefix(params, "current_abs_max"));
    current_rms_ = io_.DefineVariable<double>(with_var_prefix(params, "current_rms"));
    divb_abs_max_ = io_.DefineVariable<double>(with_var_prefix(params, "divb_abs_max"));
    divb_l2_ = io_.DefineVariable<double>(with_var_prefix(params, "divb_l2"));

    engine_ = io_.Open(params.output_file, adios2::Mode::Write);
    if (!engine_)
    {
        throw std::runtime_error("Failed to open ADIOS output: " + params.output_file);
    }
}

AdiosWriter::~AdiosWriter() { close(); }

void AdiosWriter::write(int step, double time, const OutputFields &fields,
                        const ScalarDiagnostics &diagnostics)
{
    if (closed_)
    {
        throw std::runtime_error("ADIOS writer is closed");
    }

    engine_.BeginStep();

    engine_.Put(rho_, fields.rho.data());
    engine_.Put(pressure_, fields.pressure.data());
    engine_.Put(vx_, fields.vx.data());
    engine_.Put(vy_, fields.vy.data());
    engine_.Put(vz_, fields.vz.data());
    engine_.Put(bx_, fields.bx.data());
    engine_.Put(by_, fields.by.data());
    engine_.Put(bz_, fields.bz.data());
    engine_.Put(speed_, fields.speed.data());
    engine_.Put(current_z_, fields.current_z.data());
    if (psi_)
    {
        engine_.Put(psi_, fields.psi.data());
    }
    if (mx_)
    {
        engine_.Put(mx_, fields.mx.data());
        engine_.Put(my_, fields.my.data());
        engine_.Put(mz_, fields.mz.data());
    }
    if (E_)
    {
        engine_.Put(E_, fields.E.data());
    }

    if (rank_ == 0)
    {
        engine_.Put(step_, step);
        engine_.Put(time_, time);
        engine_.Put(mass_, diagnostics.mass);
        engine_.Put(kinetic_energy_, diagnostics.kinetic_energy);
        engine_.Put(magnetic_energy_, diagnostics.magnetic_energy);
        engine_.Put(internal_energy_, diagnostics.internal_energy);
        engine_.Put(total_energy_, diagnostics.total_energy);
        engine_.Put(mean_pressure_, diagnostics.mean_pressure);
        engine_.Put(max_speed_, diagnostics.max_speed);
        engine_.Put(current_abs_max_, diagnostics.current_abs_max);
        engine_.Put(current_rms_, diagnostics.current_rms);
        engine_.Put(divb_abs_max_, diagnostics.divb_abs_max);
        engine_.Put(divb_l2_, diagnostics.divb_l2);
    }

    engine_.EndStep();
}

void AdiosWriter::close()
{
    if (!closed_ && engine_)
    {
        engine_.Close();
        closed_ = true;
    }
}

} // namespace ot
