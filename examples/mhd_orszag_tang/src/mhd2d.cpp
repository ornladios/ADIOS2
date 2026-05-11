/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mhd2d.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ot
{

namespace
{

double clamp_floor(double v, double floor) { return std::isfinite(v) ? std::max(v, floor) : floor; }

} // namespace

#ifdef OT_USE_MPI
MHD2D::MHD2D(const SimulationParams &params, MPI_Comm comm) : params_(params), comm_(comm)
{
    MPI_Comm_rank(comm_, &rank_);
    MPI_Comm_size(comm_, &size_);

    if (params_.ny < size_)
    {
        throw std::runtime_error("ny must be >= MPI size for this 1D decomposition");
    }

    const int base = params_.ny / size_;
    const int rem = params_.ny % size_;
    local_ny_ = base + (rank_ < rem ? 1 : 0);
    y_offset_ = rank_ * base + std::min(rank_, rem);

    if (local_ny_ <= 0)
    {
        throw std::runtime_error("local_ny must be positive on all ranks");
    }

    nx_tot_ = params_.nx + 2 * params_.ng;
    ny_tot_ = local_ny_ + 2 * params_.ng;

    dx_ = (params_.x_max - params_.x_min) / static_cast<double>(params_.nx);
    dy_ = (params_.y_max - params_.y_min) / static_cast<double>(params_.ny);

    const std::size_t total = static_cast<std::size_t>(kNumVars) * nx_tot_ * ny_tot_;
    u_.assign(total, 0.0);
    rhs_.assign(total, 0.0);
    stage_.assign(total, 0.0);
    rhs_stage_.assign(total, 0.0);
}
#else
MHD2D::MHD2D(const SimulationParams &params) : params_(params)
{
    local_ny_ = params_.ny;
    y_offset_ = 0;

    nx_tot_ = params_.nx + 2 * params_.ng;
    ny_tot_ = local_ny_ + 2 * params_.ng;

    dx_ = (params_.x_max - params_.x_min) / static_cast<double>(params_.nx);
    dy_ = (params_.y_max - params_.y_min) / static_cast<double>(params_.ny);

    const std::size_t total = static_cast<std::size_t>(kNumVars) * nx_tot_ * ny_tot_;
    u_.assign(total, 0.0);
    rhs_.assign(total, 0.0);
    stage_.assign(total, 0.0);
    rhs_stage_.assign(total, 0.0);
}
#endif

int MHD2D::idx(int v, int i, int j) const { return (v * ny_tot_ + j) * nx_tot_ + i; }

double MHD2D::at(const std::vector<double> &u, int v, int i, int j) const
{
    return u[idx(v, i, j)];
}

double &MHD2D::at(std::vector<double> &u, int v, int i, int j) const { return u[idx(v, i, j)]; }

MHD2D::State MHD2D::load_state(const std::vector<double> &u, int i, int j) const
{
    State s{};
    for (int v = 0; v < kNumVars; ++v)
    {
        s[v] = at(u, v, i, j);
    }
    return s;
}

void MHD2D::store_state(std::vector<double> &u, int i, int j, const State &s) const
{
    for (int v = 0; v < kNumVars; ++v)
    {
        at(u, v, i, j) = s[v];
    }
}

MHD2D::Primitive MHD2D::conserved_to_primitive(const State &s) const
{
    Primitive w{};
    w.rho = clamp_floor(s[kRho], params_.rho_floor);

    w.bx = std::isfinite(s[kBx]) ? s[kBx] : 0.0;
    w.by = std::isfinite(s[kBy]) ? s[kBy] : 0.0;
    w.bz = std::isfinite(s[kBz]) ? s[kBz] : 0.0;
    w.psi = std::isfinite(s[kPsi]) ? s[kPsi] : 0.0;

    w.vx = std::isfinite(s[kMx]) ? s[kMx] / w.rho : 0.0;
    w.vy = std::isfinite(s[kMy]) ? s[kMy] / w.rho : 0.0;
    w.vz = std::isfinite(s[kMz]) ? s[kMz] / w.rho : 0.0;

    const double v2 = w.vx * w.vx + w.vy * w.vy + w.vz * w.vz;
    const double b2 = w.bx * w.bx + w.by * w.by + w.bz * w.bz;
    const double kin = 0.5 * w.rho * v2;
    const double mag = 0.5 * b2;

    const double eint = s[kE] - kin - mag;
    w.p = clamp_floor((params_.gamma - 1.0) * eint, params_.p_floor);
    return w;
}

MHD2D::State MHD2D::primitive_to_conserved(const Primitive &w) const
{
    State s{};
    const double rho = clamp_floor(w.rho, params_.rho_floor);
    const double p = clamp_floor(w.p, params_.p_floor);

    s[kRho] = rho;
    s[kMx] = rho * w.vx;
    s[kMy] = rho * w.vy;
    s[kMz] = rho * w.vz;
    s[kBx] = w.bx;
    s[kBy] = w.by;
    s[kBz] = w.bz;

    const double v2 = w.vx * w.vx + w.vy * w.vy + w.vz * w.vz;
    const double b2 = w.bx * w.bx + w.by * w.by + w.bz * w.bz;
    s[kE] = p / (params_.gamma - 1.0) + 0.5 * rho * v2 + 0.5 * b2;
    s[kPsi] = w.psi;
    return s;
}

MHD2D::State MHD2D::flux(const State &s, int dir) const
{
    const Primitive w = conserved_to_primitive(s);

    State f{};
    const double b2 = w.bx * w.bx + w.by * w.by + w.bz * w.bz;
    const double ptot = w.p + 0.5 * b2;
    const double vdotb = w.vx * w.bx + w.vy * w.by + w.vz * w.bz;
    const double ch2 = params_.glm_ch * params_.glm_ch;

    if (dir == 0)
    {
        f[kRho] = s[kMx];
        f[kMx] = s[kMx] * w.vx + ptot - w.bx * w.bx;
        f[kMy] = s[kMy] * w.vx - w.bx * w.by;
        f[kMz] = s[kMz] * w.vx - w.bx * w.bz;

        f[kBx] = w.psi;
        f[kBy] = w.vx * w.by - w.vy * w.bx;
        f[kBz] = w.vx * w.bz - w.vz * w.bx;

        f[kE] = (s[kE] + ptot) * w.vx - vdotb * w.bx;
        f[kPsi] = ch2 * w.bx;
    }
    else
    {
        f[kRho] = s[kMy];
        f[kMx] = s[kMx] * w.vy - w.by * w.bx;
        f[kMy] = s[kMy] * w.vy + ptot - w.by * w.by;
        f[kMz] = s[kMz] * w.vy - w.by * w.bz;

        f[kBx] = w.vy * w.bx - w.vx * w.by;
        f[kBy] = w.psi;
        f[kBz] = w.vy * w.bz - w.vz * w.by;

        f[kE] = (s[kE] + ptot) * w.vy - vdotb * w.by;
        f[kPsi] = ch2 * w.by;
    }

    return f;
}

double MHD2D::fast_magnetosonic(const Primitive &w, int dir) const
{
    const double rho = std::max(w.rho, 1.0e-30);
    const double inv_rho = 1.0 / rho;
    const double a2 = params_.gamma * std::max(w.p, 0.0) * inv_rho;
    const double b2 = (w.bx * w.bx + w.by * w.by + w.bz * w.bz) * inv_rho;
    const double bn = (dir == 0 ? w.bx : w.by);
    const double bn2 = bn * bn * inv_rho;
    double disc = (a2 + b2) * (a2 + b2) - 4.0 * a2 * bn2;
    disc = std::max(disc, 0.0);
    const double cf2 = 0.5 * ((a2 + b2) + std::sqrt(disc));
    return std::sqrt(std::max(cf2, 0.0));
}

double MHD2D::signal_speed(const State &s, int dir) const
{
    const Primitive w = conserved_to_primitive(s);
    const double vn = (dir == 0 ? w.vx : w.vy);
    const double cf = fast_magnetosonic(w, dir);
    return std::abs(vn) + cf;
}

MHD2D::State MHD2D::riemann_flux(const State &left, const State &right, int dir) const
{
    const State fl = flux(left, dir);
    const State fr = flux(right, dir);

    State out{};

    if (params_.flux == FluxType::Rusanov)
    {
        const double alpha = std::max(signal_speed(left, dir), signal_speed(right, dir));
        for (int v = 0; v < kNumVars; ++v)
        {
            out[v] = 0.5 * (fl[v] + fr[v]) - 0.5 * alpha * (right[v] - left[v]);
        }
        return out;
    }

    const Primitive wl = conserved_to_primitive(left);
    const Primitive wr = conserved_to_primitive(right);

    const double vn_l = (dir == 0 ? wl.vx : wl.vy);
    const double vn_r = (dir == 0 ? wr.vx : wr.vy);

    const double cf_l = fast_magnetosonic(wl, dir);
    const double cf_r = fast_magnetosonic(wr, dir);

    const double s_l = std::min(vn_l - cf_l, vn_r - cf_r);
    const double s_r = std::max(vn_l + cf_l, vn_r + cf_r);

    if (s_l >= 0.0)
    {
        return fl;
    }
    if (s_r <= 0.0)
    {
        return fr;
    }

    const double inv = 1.0 / (s_r - s_l);
    for (int v = 0; v < kNumVars; ++v)
    {
        out[v] = (s_r * fl[v] - s_l * fr[v] + s_l * s_r * (right[v] - left[v])) * inv;
    }
    return out;
}

double MHD2D::minmod(double a, double b)
{
    if (a * b <= 0.0)
    {
        return 0.0;
    }
    return (std::abs(a) < std::abs(b)) ? a : b;
}

void MHD2D::fill_boundaries(std::vector<double> &u) const
{
    const int ng = params_.ng;

    // x periodic boundaries (local).
    for (int v = 0; v < kNumVars; ++v)
    {
        for (int j = 0; j < ny_tot_; ++j)
        {
            for (int g = 0; g < ng; ++g)
            {
                at(u, v, g, j) = at(u, v, params_.nx + g, j);
                at(u, v, ng + params_.nx + g, j) = at(u, v, ng + g, j);
            }
        }
    }

#ifdef OT_USE_MPI
    if (size_ > 1)
    {
        const int prev = (rank_ - 1 + size_) % size_;
        const int next = (rank_ + 1) % size_;

        const int count = kNumVars * ng * nx_tot_;

        std::vector<double> send_bottom(count, 0.0);
        std::vector<double> send_top(count, 0.0);
        std::vector<double> recv_bottom(count, 0.0);
        std::vector<double> recv_top(count, 0.0);

        int c = 0;
        for (int v = 0; v < kNumVars; ++v)
        {
            for (int g = 0; g < ng; ++g)
            {
                const int j = ng + g;
                for (int i = 0; i < nx_tot_; ++i)
                {
                    send_bottom[c++] = at(u, v, i, j);
                }
            }
        }

        c = 0;
        for (int v = 0; v < kNumVars; ++v)
        {
            for (int g = 0; g < ng; ++g)
            {
                const int j = local_ny_ + g;
                for (int i = 0; i < nx_tot_; ++i)
                {
                    send_top[c++] = at(u, v, i, j);
                }
            }
        }

        MPI_Sendrecv(send_bottom.data(), count, MPI_DOUBLE, prev, 101, recv_top.data(), count,
                     MPI_DOUBLE, next, 101, comm_, MPI_STATUS_IGNORE);

        MPI_Sendrecv(send_top.data(), count, MPI_DOUBLE, next, 102, recv_bottom.data(), count,
                     MPI_DOUBLE, prev, 102, comm_, MPI_STATUS_IGNORE);

        c = 0;
        for (int v = 0; v < kNumVars; ++v)
        {
            for (int g = 0; g < ng; ++g)
            {
                const int j = g;
                for (int i = 0; i < nx_tot_; ++i)
                {
                    at(u, v, i, j) = recv_bottom[c++];
                }
            }
        }

        c = 0;
        for (int v = 0; v < kNumVars; ++v)
        {
            for (int g = 0; g < ng; ++g)
            {
                const int j = ng + local_ny_ + g;
                for (int i = 0; i < nx_tot_; ++i)
                {
                    at(u, v, i, j) = recv_top[c++];
                }
            }
        }
        return;
    }
#endif

    // y periodic boundaries (serial or 1 rank).
    for (int v = 0; v < kNumVars; ++v)
    {
        for (int i = 0; i < nx_tot_; ++i)
        {
            for (int g = 0; g < ng; ++g)
            {
                at(u, v, i, g) = at(u, v, i, local_ny_ + g);
                at(u, v, i, ng + local_ny_ + g) = at(u, v, i, ng + g);
            }
        }
    }
}

void MHD2D::enforce_floors(std::vector<double> &u) const
{
    const int ng = params_.ng;
    for (int j = ng; j < ng + local_ny_; ++j)
    {
        for (int i = ng; i < ng + params_.nx; ++i)
        {
            State s = load_state(u, i, j);
            Primitive w = conserved_to_primitive(s);
            s = primitive_to_conserved(w);
            store_state(u, i, j, s);
        }
    }
}

void MHD2D::compute_rhs(const std::vector<double> &u, std::vector<double> &rhs,
                        bool second_order) const
{
    std::fill(rhs.begin(), rhs.end(), 0.0);

    const int ng = params_.ng;
    const int nx = params_.nx;
    const int ny = local_ny_;

    std::vector<double> fx(static_cast<std::size_t>(kNumVars) * (nx + 1) * ny, 0.0);
    std::vector<double> fy(static_cast<std::size_t>(kNumVars) * nx * (ny + 1), 0.0);

    auto fx_idx = [nx, ny](int v, int iface, int jlocal) {
        return (v * ny + jlocal) * (nx + 1) + iface;
    };

    auto fy_idx = [nx, ny](int v, int ilocal, int jface) {
        return (v * (ny + 1) + jface) * nx + ilocal;
    };

    auto regularize = [this](State &s) {
        Primitive w = conserved_to_primitive(s);
        s = primitive_to_conserved(w);
    };

    for (int jlocal = 0; jlocal < ny; ++jlocal)
    {
        const int j = ng + jlocal;
        for (int iface = 0; iface <= nx; ++iface)
        {
            const int i_right = ng + iface;
            const int i_left = i_right - 1;

            State left = load_state(u, i_left, j);
            State right = load_state(u, i_right, j);

            if (second_order)
            {
                for (int v = 0; v < kNumVars; ++v)
                {
                    const double dl_l = at(u, v, i_left, j) - at(u, v, i_left - 1, j);
                    const double dr_l = at(u, v, i_left + 1, j) - at(u, v, i_left, j);
                    const double slope_l = minmod(dl_l, dr_l);

                    const double dl_r = at(u, v, i_right, j) - at(u, v, i_right - 1, j);
                    const double dr_r = at(u, v, i_right + 1, j) - at(u, v, i_right, j);
                    const double slope_r = minmod(dl_r, dr_r);

                    left[v] += 0.5 * slope_l;
                    right[v] -= 0.5 * slope_r;
                }
            }

            regularize(left);
            regularize(right);
            const State f = riemann_flux(left, right, 0);
            for (int v = 0; v < kNumVars; ++v)
            {
                fx[fx_idx(v, iface, jlocal)] = f[v];
            }
        }
    }

    for (int jface = 0; jface <= ny; ++jface)
    {
        const int j_top = ng + jface;
        const int j_bottom = j_top - 1;

        for (int ilocal = 0; ilocal < nx; ++ilocal)
        {
            const int i = ng + ilocal;

            State bottom = load_state(u, i, j_bottom);
            State top = load_state(u, i, j_top);

            if (second_order)
            {
                for (int v = 0; v < kNumVars; ++v)
                {
                    const double dl_b = at(u, v, i, j_bottom) - at(u, v, i, j_bottom - 1);
                    const double dr_b = at(u, v, i, j_bottom + 1) - at(u, v, i, j_bottom);
                    const double slope_b = minmod(dl_b, dr_b);

                    const double dl_t = at(u, v, i, j_top) - at(u, v, i, j_top - 1);
                    const double dr_t = at(u, v, i, j_top + 1) - at(u, v, i, j_top);
                    const double slope_t = minmod(dl_t, dr_t);

                    bottom[v] += 0.5 * slope_b;
                    top[v] -= 0.5 * slope_t;
                }
            }

            regularize(bottom);
            regularize(top);
            const State g = riemann_flux(bottom, top, 1);
            for (int v = 0; v < kNumVars; ++v)
            {
                fy[fy_idx(v, ilocal, jface)] = g[v];
            }
        }
    }

    for (int jlocal = 0; jlocal < ny; ++jlocal)
    {
        const int j = ng + jlocal;
        for (int ilocal = 0; ilocal < nx; ++ilocal)
        {
            const int i = ng + ilocal;
            for (int v = 0; v < kNumVars; ++v)
            {
                const double dfx =
                    (fx[fx_idx(v, ilocal + 1, jlocal)] - fx[fx_idx(v, ilocal, jlocal)]) / dx_;
                const double dfy =
                    (fy[fy_idx(v, ilocal, jlocal + 1)] - fy[fy_idx(v, ilocal, jlocal)]) / dy_;
                at(rhs, v, i, j) = -(dfx + dfy);
            }
            at(rhs, kPsi, i, j) += -params_.glm_damping * at(u, kPsi, i, j);
        }
    }
}

void MHD2D::initialize_orszag_tang()
{
    const int ng = params_.ng;
    constexpr double pi = 3.14159265358979323846;

    for (int jlocal = 0; jlocal < local_ny_; ++jlocal)
    {
        const int j = ng + jlocal;
        const int jg = y_offset_ + jlocal;

        for (int ilocal = 0; ilocal < params_.nx; ++ilocal)
        {
            const int i = ng + ilocal;

            const double x = params_.x_min + (static_cast<double>(ilocal) + 0.5) * dx_;
            const double y = params_.y_min + (static_cast<double>(jg) + 0.5) * dy_;

            Primitive w{};
            w.rho = 25.0 / (36.0 * pi);
            w.p = 5.0 / (12.0 * pi);
            w.vx = -std::sin(y);
            w.vy = std::sin(x);
            w.vz = 0.0;
            w.bx = -std::sin(y);
            w.by = std::sin(2.0 * x);
            w.bz = 0.0;
            w.psi = 0.0;

            const State s = primitive_to_conserved(w);
            store_state(u_, i, j, s);
        }
    }

    fill_boundaries(u_);
}

double MHD2D::compute_time_step() const
{
    if (params_.fixed_dt > 0.0)
    {
        return params_.fixed_dt;
    }

    const int ng = params_.ng;
    double local_max = 1.0e-12;

    for (int j = ng; j < ng + local_ny_; ++j)
    {
        for (int i = ng; i < ng + params_.nx; ++i)
        {
            const State s = load_state(u_, i, j);
            const double sx = signal_speed(s, 0);
            const double sy = signal_speed(s, 1);
            local_max = std::max(local_max, sx);
            local_max = std::max(local_max, sy);
        }
    }

    local_max = std::max(local_max, params_.glm_ch);

    double global_max = local_max;
#ifdef OT_USE_MPI
    if (size_ > 1)
    {
        MPI_Allreduce(&local_max, &global_max, 1, MPI_DOUBLE, MPI_MAX, comm_);
    }
#endif

    const double dt = params_.cfl * std::min(dx_, dy_) / std::max(global_max, 1.0e-12);
    return dt;
}

void MHD2D::advance(double dt)
{
    const int ng = params_.ng;

    fill_boundaries(u_);

    if (params_.reconstruction == ReconstructionType::FirstOrder)
    {
        compute_rhs(u_, rhs_, false);

        for (int j = ng; j < ng + local_ny_; ++j)
        {
            for (int i = ng; i < ng + params_.nx; ++i)
            {
                for (int v = 0; v < kNumVars; ++v)
                {
                    at(u_, v, i, j) += dt * at(rhs_, v, i, j);
                }
            }
        }

        enforce_floors(u_);
        fill_boundaries(u_);
        return;
    }

    compute_rhs(u_, rhs_, true);

    stage_ = u_;
    for (int j = ng; j < ng + local_ny_; ++j)
    {
        for (int i = ng; i < ng + params_.nx; ++i)
        {
            for (int v = 0; v < kNumVars; ++v)
            {
                at(stage_, v, i, j) += dt * at(rhs_, v, i, j);
            }
        }
    }

    enforce_floors(stage_);
    fill_boundaries(stage_);
    compute_rhs(stage_, rhs_stage_, true);

    for (int j = ng; j < ng + local_ny_; ++j)
    {
        for (int i = ng; i < ng + params_.nx; ++i)
        {
            for (int v = 0; v < kNumVars; ++v)
            {
                const double un = at(u_, v, i, j);
                const double u1 = at(stage_, v, i, j);
                const double rhs1 = at(rhs_stage_, v, i, j);
                at(u_, v, i, j) = 0.5 * (un + u1 + dt * rhs1);
            }
        }
    }

    enforce_floors(u_);
    fill_boundaries(u_);
}

void MHD2D::extract_output(OutputFields &fields) const
{
    const std::size_t local_size = static_cast<std::size_t>(params_.nx) * local_ny_;

    fields.rho.assign(local_size, 0.0);
    fields.pressure.assign(local_size, 0.0);
    fields.vx.assign(local_size, 0.0);
    fields.vy.assign(local_size, 0.0);
    fields.vz.assign(local_size, 0.0);
    fields.bx.assign(local_size, 0.0);
    fields.by.assign(local_size, 0.0);
    fields.bz.assign(local_size, 0.0);
    fields.speed.assign(local_size, 0.0);
    fields.current_z.assign(local_size, 0.0);
    if (params_.output_psi)
    {
        fields.psi.assign(local_size, 0.0);
    }
    else
    {
        fields.psi.clear();
    }
    if (params_.output_m)
    {
        fields.mx.assign(local_size, 0.0);
        fields.my.assign(local_size, 0.0);
        fields.mz.assign(local_size, 0.0);
    }
    else
    {
        fields.mx.clear();
        fields.my.clear();
        fields.mz.clear();
    }
    if (params_.output_E)
    {
        fields.E.assign(local_size, 0.0);
    }
    else
    {
        fields.E.clear();
    }

    const int ng = params_.ng;

    for (int jlocal = 0; jlocal < local_ny_; ++jlocal)
    {
        const int j = ng + jlocal;

        for (int ilocal = 0; ilocal < params_.nx; ++ilocal)
        {
            const int i = ng + ilocal;
            const std::size_t out = static_cast<std::size_t>(jlocal) * params_.nx + ilocal;

            const State s = load_state(u_, i, j);
            const Primitive w = conserved_to_primitive(s);

            fields.rho[out] = w.rho;
            fields.pressure[out] = w.p;
            fields.vx[out] = w.vx;
            fields.vy[out] = w.vy;
            fields.vz[out] = w.vz;
            fields.bx[out] = w.bx;
            fields.by[out] = w.by;
            fields.bz[out] = w.bz;
            fields.speed[out] = std::sqrt(w.vx * w.vx + w.vy * w.vy + w.vz * w.vz);
            if (params_.output_psi)
            {
                fields.psi[out] = w.psi;
            }
            if (params_.output_m)
            {
                fields.mx[out] = s[kMx];
                fields.my[out] = s[kMy];
                fields.mz[out] = s[kMz];
            }
            if (params_.output_E)
            {
                fields.E[out] = s[kE];
            }

            const double dby_dx = (at(u_, kBy, i + 1, j) - at(u_, kBy, i - 1, j)) / (2.0 * dx_);
            const double dbx_dy = (at(u_, kBx, i, j + 1) - at(u_, kBx, i, j - 1)) / (2.0 * dy_);
            fields.current_z[out] = dby_dx - dbx_dy;
        }
    }
}

void MHD2D::compute_diagnostics(ScalarDiagnostics &diagnostics) const
{
    const int ng = params_.ng;
    const std::size_t global_cells =
        static_cast<std::size_t>(params_.nx) * static_cast<std::size_t>(params_.ny);

    double sum_rho = 0.0;
    double sum_kin = 0.0;
    double sum_mag = 0.0;
    double sum_int = 0.0;
    double sum_tot = 0.0;
    double sum_p = 0.0;
    double sum_j2 = 0.0;
    double sum_divb2 = 0.0;
    double max_speed = 0.0;
    double max_jabs = 0.0;
    double max_divb_abs = 0.0;

    for (int jlocal = 0; jlocal < local_ny_; ++jlocal)
    {
        const int j = ng + jlocal;
        for (int ilocal = 0; ilocal < params_.nx; ++ilocal)
        {
            const int i = ng + ilocal;

            const State s = load_state(u_, i, j);
            const Primitive w = conserved_to_primitive(s);

            const double v2 = w.vx * w.vx + w.vy * w.vy + w.vz * w.vz;
            const double b2 = w.bx * w.bx + w.by * w.by + w.bz * w.bz;
            const double speed = std::sqrt(v2);

            sum_rho += w.rho;
            sum_kin += 0.5 * w.rho * v2;
            sum_mag += 0.5 * b2;
            sum_int += w.p / (params_.gamma - 1.0);
            sum_tot += s[kE];
            sum_p += w.p;
            max_speed = std::max(max_speed, speed);

            const double dby_dx = (at(u_, kBy, i + 1, j) - at(u_, kBy, i - 1, j)) / (2.0 * dx_);
            const double dbx_dy = (at(u_, kBx, i, j + 1) - at(u_, kBx, i, j - 1)) / (2.0 * dy_);
            const double current_z = dby_dx - dbx_dy;
            const double jabs = std::abs(current_z);
            sum_j2 += current_z * current_z;
            max_jabs = std::max(max_jabs, jabs);

            const double dbx_dx = (at(u_, kBx, i + 1, j) - at(u_, kBx, i - 1, j)) / (2.0 * dx_);
            const double dby_dy = (at(u_, kBy, i, j + 1) - at(u_, kBy, i, j - 1)) / (2.0 * dy_);
            const double divb = dbx_dx + dby_dy;
            const double divb_abs = std::abs(divb);
            sum_divb2 += divb * divb;
            max_divb_abs = std::max(max_divb_abs, divb_abs);
        }
    }

    std::array<double, 8> sums_local = {sum_rho, sum_kin, sum_mag, sum_int,
                                        sum_tot, sum_p,   sum_j2,  sum_divb2};
    std::array<double, 3> max_local = {max_speed, max_jabs, max_divb_abs};

    std::array<double, 8> sums_global = sums_local;
    std::array<double, 3> max_global = max_local;

#ifdef OT_USE_MPI
    if (size_ > 1)
    {
        MPI_Allreduce(sums_local.data(), sums_global.data(), static_cast<int>(sums_local.size()),
                      MPI_DOUBLE, MPI_SUM, comm_);
        MPI_Allreduce(max_local.data(), max_global.data(), static_cast<int>(max_local.size()),
                      MPI_DOUBLE, MPI_MAX, comm_);
    }
#endif

    const double cell_area = dx_ * dy_;
    const double inv_cells = 1.0 / static_cast<double>(global_cells);

    diagnostics.mass = sums_global[0] * cell_area;
    diagnostics.kinetic_energy = sums_global[1] * cell_area;
    diagnostics.magnetic_energy = sums_global[2] * cell_area;
    diagnostics.internal_energy = sums_global[3] * cell_area;
    diagnostics.total_energy = sums_global[4] * cell_area;
    diagnostics.mean_pressure = sums_global[5] * inv_cells;
    diagnostics.max_speed = max_global[0];
    diagnostics.current_abs_max = max_global[1];
    diagnostics.current_rms = std::sqrt(sums_global[6] * inv_cells);
    diagnostics.divb_abs_max = max_global[2];
    diagnostics.divb_l2 = std::sqrt(sums_global[7] * inv_cells);
}

std::string flux_to_string(FluxType flux)
{
    return (flux == FluxType::Rusanov) ? "rusanov" : "hll";
}

std::string reconstruction_to_string(ReconstructionType recon)
{
    return (recon == ReconstructionType::FirstOrder) ? "first_order" : "muscl";
}

bool parse_solver_name(const std::string &name, FluxType &flux, ReconstructionType &recon)
{
    std::string s = name;
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (s == "rusanov" || s == "rusanov1" || s == "llf" || s == "first_order_rusanov")
    {
        flux = FluxType::Rusanov;
        recon = ReconstructionType::FirstOrder;
        return true;
    }

    if (s == "hll" || s == "hll1" || s == "first_order_hll")
    {
        flux = FluxType::HLL;
        recon = ReconstructionType::FirstOrder;
        return true;
    }

    if (s == "muscl" || s == "muscl_hll" || s == "muscl-hll" || s == "rk2_hll")
    {
        flux = FluxType::HLL;
        recon = ReconstructionType::MUSCL;
        return true;
    }

    if (s == "muscl_rusanov" || s == "muscl-rusanov" || s == "rk2_rusanov")
    {
        flux = FluxType::Rusanov;
        recon = ReconstructionType::MUSCL;
        return true;
    }

    return false;
}

} // namespace ot
