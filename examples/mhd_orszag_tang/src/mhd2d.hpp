/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <limits>
#include <string>
#include <vector>

#ifdef OT_USE_MPI
#include <mpi.h>
#endif

namespace ot
{

constexpr int kNumVars = 9;

constexpr int kRho = 0;
constexpr int kMx = 1;
constexpr int kMy = 2;
constexpr int kMz = 3;
constexpr int kBx = 4;
constexpr int kBy = 5;
constexpr int kBz = 6;
constexpr int kE = 7;
constexpr int kPsi = 8;

enum class FluxType
{
    Rusanov,
    HLL,
};

enum class ReconstructionType
{
    FirstOrder,
    MUSCL,
};

struct SimulationParams
{
    int nx = 256;
    int ny = 256;
    int ng = 2;
    int max_steps = 250000;
    int log_every_steps = 50;

    double x_min = 0.0;
    double x_max = 6.2831853071795864769; // 2*pi
    double y_min = 0.0;
    double y_max = 6.2831853071795864769; // 2*pi

    double gamma = 5.0 / 3.0;
    double cfl = 0.35;
    double tfinal = 1.0;

    double rho_floor = 1.0e-8;
    double p_floor = 1.0e-8;

    double glm_ch = 1.0;
    double glm_damping = 0.1;

    FluxType flux = FluxType::HLL;
    ReconstructionType reconstruction = ReconstructionType::MUSCL;

    std::string output_file = "output/output.bp";
    std::string adios_engine = "BPFile";
    std::string prepend_var_names;
    double fixed_dt = 0.0;
    bool output_psi = false;
    bool output_m = false;
    bool output_E = false;
    int output_every_steps = 20;
    double output_every_time = 0.0;
    bool save_initial = true;
};

struct OutputFields
{
    std::vector<double> rho;
    std::vector<double> pressure;
    std::vector<double> vx;
    std::vector<double> vy;
    std::vector<double> vz;
    std::vector<double> bx;
    std::vector<double> by;
    std::vector<double> bz;
    std::vector<double> speed;
    std::vector<double> current_z;
    std::vector<double> psi;
    std::vector<double> mx;
    std::vector<double> my;
    std::vector<double> mz;
    std::vector<double> E;
};

struct ScalarDiagnostics
{
    double mass = 0.0;
    double kinetic_energy = 0.0;
    double magnetic_energy = 0.0;
    double internal_energy = 0.0;
    double total_energy = 0.0;
    double mean_pressure = 0.0;
    double max_speed = 0.0;
    double current_abs_max = 0.0;
    double current_rms = 0.0;
    double divb_abs_max = 0.0;
    double divb_l2 = 0.0;
};

class MHD2D
{
public:
#ifdef OT_USE_MPI
    MHD2D(const SimulationParams &params, MPI_Comm comm);
#else
    explicit MHD2D(const SimulationParams &params);
#endif

    void initialize_orszag_tang();
    double compute_time_step() const;
    void advance(double dt);
    void extract_output(OutputFields &fields) const;
    void compute_diagnostics(ScalarDiagnostics &diagnostics) const;

    int global_nx() const { return params_.nx; }
    int global_ny() const { return params_.ny; }
    int local_ny() const { return local_ny_; }
    int y_offset() const { return y_offset_; }
    int rank() const { return rank_; }
    int size() const { return size_; }

private:
    using State = std::array<double, kNumVars>;

    struct Primitive
    {
        double rho;
        double vx;
        double vy;
        double vz;
        double bx;
        double by;
        double bz;
        double p;
        double psi;
    };

    int idx(int v, int i, int j) const;
    double at(const std::vector<double> &u, int v, int i, int j) const;
    double &at(std::vector<double> &u, int v, int i, int j) const;

    State load_state(const std::vector<double> &u, int i, int j) const;
    void store_state(std::vector<double> &u, int i, int j, const State &s) const;

    Primitive conserved_to_primitive(const State &s) const;
    State primitive_to_conserved(const Primitive &w) const;

    State flux(const State &s, int dir) const;
    State riemann_flux(const State &left, const State &right, int dir) const;
    double fast_magnetosonic(const Primitive &w, int dir) const;
    double signal_speed(const State &s, int dir) const;

    static double minmod(double a, double b);

    void fill_boundaries(std::vector<double> &u) const;
    void enforce_floors(std::vector<double> &u) const;
    void compute_rhs(const std::vector<double> &u, std::vector<double> &rhs,
                     bool second_order) const;

private:
    SimulationParams params_;

#ifdef OT_USE_MPI
    MPI_Comm comm_;
#endif

    int rank_ = 0;
    int size_ = 1;

    int local_ny_ = 0;
    int y_offset_ = 0;

    int nx_tot_ = 0;
    int ny_tot_ = 0;

    double dx_ = 0.0;
    double dy_ = 0.0;

    std::vector<double> u_;
    std::vector<double> rhs_;
    std::vector<double> stage_;
    std::vector<double> rhs_stage_;
};

std::string flux_to_string(FluxType flux);
std::string reconstruction_to_string(ReconstructionType recon);
bool parse_solver_name(const std::string &name, FluxType &flux, ReconstructionType &recon);

} // namespace ot
