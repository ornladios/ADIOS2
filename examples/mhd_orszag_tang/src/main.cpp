/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adios_writer.hpp"
#include "mhd2d.hpp"

#include <cerrno>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <direct.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#ifdef OT_USE_MPI
#include <mpi.h>
#endif

namespace
{

#ifdef _WIN32
constexpr char kPathSeparator = '\\';
int make_directory(const std::string &path) { return _mkdir(path.c_str()); }
#else
constexpr char kPathSeparator = '/';
int make_directory(const std::string &path) { return mkdir(path.c_str(), 0777); }
#endif

bool is_directory(const std::string &path)
{
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR) != 0;
}

std::string parent_path(const std::string &path)
{
    const std::string::size_type pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
    {
        return "";
    }
    if (pos == 0)
    {
        return path.substr(0, 1);
    }
    return path.substr(0, pos);
}

std::string join_path(const std::string &directory, const std::string &file)
{
    if (directory.empty() || directory == ".")
    {
        return file;
    }
    const char last = directory[directory.size() - 1];
    if (last == '/' || last == '\\')
    {
        return directory + file;
    }
    return directory + kPathSeparator + file;
}

void create_directories(const std::string &path)
{
    if (path.empty() || path == "." || is_directory(path))
    {
        return;
    }

    const std::string parent = parent_path(path);
    if (!parent.empty() && parent != path)
    {
        create_directories(parent);
    }

    errno = 0;
    if (make_directory(path) != 0 && !is_directory(path))
    {
        throw std::runtime_error("Failed to create directory: " + path);
    }
}

void print_usage(const char *prog)
{
    std::cout
        << "Usage: " << prog << " [options]\n\n"
        << "Core options:\n"
        << "  --nx <int>                    Global x resolution (default 256)\n"
        << "  --ny <int>                    Global y resolution (default 256)\n"
        << "  --tfinal <float>              Final simulation time (default 1.0)\n"
        << "  --cfl <float>                 CFL number (default 0.35)\n"
        << "  --gamma <float>               Ratio of specific heats (default 5/3)\n"
        << "  --solver <name>               rusanov|hll|muscl_hll|muscl_rusanov\n"
        << "  --flux <name>                 rusanov|hll (optional override)\n"
        << "  --reconstruction <name>       first|muscl (optional override)\n"
        << "\nOutput options:\n"
        << "  --output <path>               ADIOS2 BP output file (default output/output.bp)\n"
        << "  --output-dir <path>           Run output directory (writes <path>/output.bp)\n"
        << "  --engine <name>               ADIOS2 engine (default BPFile)\n"
        << "  --prepend-var-names <str>     Prefix added to all ADIOS variable names\n"
        << "  --fixed-dt <float>            Fixed timestep (overrides CFL-based dt)\n"
        << "  --psi                         Output the GLM cleaning field psi\n"
        << "  --m                           Output momentum fields mx,my,mz\n"
        << "  --E                           Output total energy density field E\n"
        << "  --output-every-steps <int>    Save every N steps (0 disables)\n"
        << "  --output-every-time <float>   Save every Dt in sim time (0 disables)\n"
        << "  --save-initial                Save initial state (default true)\n"
        << "  --no-save-initial             Skip initial output\n"
        << "\nStability / control options:\n"
        << "  --rho-floor <float>           Density floor\n"
        << "  --p-floor <float>             Pressure floor\n"
        << "  --glm-ch <float>              GLM propagation speed\n"
        << "  --glm-damping <float>         GLM damping coefficient\n"
        << "  --max-steps <int>             Hard step limit\n"
        << "  --log-every-steps <int>       Print progress every N steps\n"
        << "  --help                        Show this help\n";
}

bool parse_int_arg(const std::string &arg, int &out)
{
    try
    {
        out = std::stoi(arg);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool parse_double_arg(const std::string &arg, double &out)
{
    try
    {
        out = std::stod(arg);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::string next_value(int &i, int argc, char **argv, const std::string &opt)
{
    if (i + 1 >= argc)
    {
        throw std::runtime_error("Missing value for option " + opt);
    }
    ++i;
    return argv[i];
}

void write_input_parameters(const std::string &params_file, const std::string &output_dir,
                            const ot::SimulationParams &params)
{
    std::ofstream out(params_file);
    if (!out)
    {
        throw std::runtime_error("Failed to open parameter file: " + params_file);
    }

    out << std::boolalpha << std::setprecision(17);
    out << "nx=" << params.nx << "\n";
    out << "ny=" << params.ny << "\n";
    out << "ng=" << params.ng << "\n";
    out << "tfinal=" << params.tfinal << "\n";
    out << "cfl=" << params.cfl << "\n";
    out << "gamma=" << params.gamma << "\n";
    out << "x_min=" << params.x_min << "\n";
    out << "x_max=" << params.x_max << "\n";
    out << "y_min=" << params.y_min << "\n";
    out << "y_max=" << params.y_max << "\n";
    out << "solver=" << ot::reconstruction_to_string(params.reconstruction) << "+"
        << ot::flux_to_string(params.flux) << "\n";
    out << "flux=" << ot::flux_to_string(params.flux) << "\n";
    out << "reconstruction=" << ot::reconstruction_to_string(params.reconstruction) << "\n";
    out << "rho_floor=" << params.rho_floor << "\n";
    out << "p_floor=" << params.p_floor << "\n";
    out << "glm_ch=" << params.glm_ch << "\n";
    out << "glm_damping=" << params.glm_damping << "\n";
    out << "max_steps=" << params.max_steps << "\n";
    out << "log_every_steps=" << params.log_every_steps << "\n";
    out << "adios_engine=" << params.adios_engine << "\n";
    out << "prepend_var_names=" << params.prepend_var_names << "\n";
    out << "fixed_dt=" << params.fixed_dt << "\n";
    out << "output_psi=" << params.output_psi << "\n";
    out << "output_m=" << params.output_m << "\n";
    out << "output_E=" << params.output_E << "\n";
    out << "output_every_steps=" << params.output_every_steps << "\n";
    out << "output_every_time=" << params.output_every_time << "\n";
    out << "save_initial=" << params.save_initial << "\n";
    out << "output_dir=" << output_dir << "\n";
    out << "output_file=" << params.output_file << "\n";
}

} // namespace

int main(int argc, char **argv)
{
#ifdef OT_USE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;
    int rank = 0;
    MPI_Comm_rank(comm, &rank);
#else
    int rank = 0;
#endif

    try
    {
        ot::SimulationParams params;
        bool output_file_set = false;
        bool output_dir_set = false;
        std::string output_dir_arg;

        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];

            if (arg == "--help")
            {
                if (rank == 0)
                {
                    print_usage(argv[0]);
                }
#ifdef OT_USE_MPI
                MPI_Finalize();
#endif
                return 0;
            }
            if (arg == "--nx")
            {
                int v = 0;
                if (!parse_int_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid integer for --nx");
                }
                params.nx = v;
                continue;
            }
            if (arg == "--ny")
            {
                int v = 0;
                if (!parse_int_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid integer for --ny");
                }
                params.ny = v;
                continue;
            }
            if (arg == "--tfinal")
            {
                double v = 0.0;
                if (!parse_double_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid float for --tfinal");
                }
                params.tfinal = v;
                continue;
            }
            if (arg == "--cfl")
            {
                double v = 0.0;
                if (!parse_double_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid float for --cfl");
                }
                params.cfl = v;
                continue;
            }
            if (arg == "--gamma")
            {
                double v = 0.0;
                if (!parse_double_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid float for --gamma");
                }
                params.gamma = v;
                continue;
            }
            if (arg == "--solver")
            {
                const std::string solver_name = next_value(i, argc, argv, arg);
                if (!ot::parse_solver_name(solver_name, params.flux, params.reconstruction))
                {
                    throw std::runtime_error("Unknown solver: " + solver_name);
                }
                continue;
            }
            if (arg == "--flux")
            {
                const std::string flux_name = next_value(i, argc, argv, arg);
                if (flux_name == "rusanov")
                {
                    params.flux = ot::FluxType::Rusanov;
                }
                else if (flux_name == "hll")
                {
                    params.flux = ot::FluxType::HLL;
                }
                else
                {
                    throw std::runtime_error("Unknown flux: " + flux_name);
                }
                continue;
            }
            if (arg == "--reconstruction")
            {
                const std::string recon_name = next_value(i, argc, argv, arg);
                if (recon_name == "first" || recon_name == "first_order")
                {
                    params.reconstruction = ot::ReconstructionType::FirstOrder;
                }
                else if (recon_name == "muscl")
                {
                    params.reconstruction = ot::ReconstructionType::MUSCL;
                }
                else
                {
                    throw std::runtime_error("Unknown reconstruction: " + recon_name);
                }
                continue;
            }
            if (arg == "--output")
            {
                params.output_file = next_value(i, argc, argv, arg);
                output_file_set = true;
                continue;
            }
            if (arg == "--output-dir")
            {
                output_dir_arg = next_value(i, argc, argv, arg);
                output_dir_set = true;
                continue;
            }
            if (arg == "--engine")
            {
                params.adios_engine = next_value(i, argc, argv, arg);
                continue;
            }
            if (arg == "--prepend-var-names" || arg == "--prepend_var_names")
            {
                params.prepend_var_names = next_value(i, argc, argv, arg);
                continue;
            }
            if (arg == "--fixed-dt" || arg == "--fixed_dt")
            {
                double v = 0.0;
                if (!parse_double_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid float for --fixed-dt");
                }
                params.fixed_dt = v;
                continue;
            }
            if (arg == "--psi")
            {
                params.output_psi = true;
                continue;
            }
            if (arg == "--m")
            {
                params.output_m = true;
                continue;
            }
            if (arg == "--E" || arg == "--e")
            {
                params.output_E = true;
                continue;
            }
            if (arg == "--output-every-steps")
            {
                int v = 0;
                if (!parse_int_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid integer for --output-every-steps");
                }
                params.output_every_steps = v;
                continue;
            }
            if (arg == "--output-every-time")
            {
                double v = 0.0;
                if (!parse_double_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid float for --output-every-time");
                }
                params.output_every_time = v;
                continue;
            }
            if (arg == "--save-initial")
            {
                params.save_initial = true;
                continue;
            }
            if (arg == "--no-save-initial")
            {
                params.save_initial = false;
                continue;
            }
            if (arg == "--rho-floor")
            {
                double v = 0.0;
                if (!parse_double_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid float for --rho-floor");
                }
                params.rho_floor = v;
                continue;
            }
            if (arg == "--p-floor")
            {
                double v = 0.0;
                if (!parse_double_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid float for --p-floor");
                }
                params.p_floor = v;
                continue;
            }
            if (arg == "--glm-ch")
            {
                double v = 0.0;
                if (!parse_double_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid float for --glm-ch");
                }
                params.glm_ch = v;
                continue;
            }
            if (arg == "--glm-damping")
            {
                double v = 0.0;
                if (!parse_double_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid float for --glm-damping");
                }
                params.glm_damping = v;
                continue;
            }
            if (arg == "--max-steps")
            {
                int v = 0;
                if (!parse_int_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid integer for --max-steps");
                }
                params.max_steps = v;
                continue;
            }
            if (arg == "--log-every-steps")
            {
                int v = 0;
                if (!parse_int_arg(next_value(i, argc, argv, arg), v))
                {
                    throw std::runtime_error("Invalid integer for --log-every-steps");
                }
                params.log_every_steps = v;
                continue;
            }

            throw std::runtime_error("Unknown option: " + arg);
        }

        if (params.nx <= 4 || params.ny <= 4)
        {
            throw std::runtime_error("nx and ny must be > 4");
        }
        if (params.cfl <= 0.0)
        {
            throw std::runtime_error("cfl must be > 0");
        }
        if (params.tfinal <= 0.0)
        {
            throw std::runtime_error("tfinal must be > 0");
        }
        if (params.output_every_steps < 0 || params.output_every_time < 0.0)
        {
            throw std::runtime_error("output cadence values must be non-negative");
        }
        if (params.fixed_dt < 0.0)
        {
            throw std::runtime_error("fixed_dt must be non-negative");
        }

        if (output_file_set && output_dir_set)
        {
            throw std::runtime_error("Use either --output or --output-dir, not both");
        }

        std::string run_output_dir;
        if (output_dir_set)
        {
            run_output_dir = output_dir_arg;
            params.output_file = join_path(run_output_dir, "output.bp");
        }
        else
        {
            const std::string explicit_parent = parent_path(params.output_file);
            run_output_dir = !explicit_parent.empty() ? explicit_parent : ".";
        }

        const std::string output_parent = parent_path(params.output_file);
        const std::string params_file = join_path(run_output_dir, "input_parameters.txt");
        if (rank == 0)
        {
            create_directories(run_output_dir);
            if (!output_parent.empty())
            {
                create_directories(output_parent);
            }
            write_input_parameters(params_file, run_output_dir, params);
        }
#ifdef OT_USE_MPI
        MPI_Barrier(comm);
#endif

#ifdef OT_USE_MPI
        ot::MHD2D sim(params, comm);
#else
        ot::MHD2D sim(params);
#endif
        sim.initialize_orszag_tang();

#ifdef OT_USE_MPI
        ot::AdiosWriter writer(params, sim.global_nx(), sim.global_ny(), sim.local_ny(),
                               sim.y_offset(), sim.rank(), comm);
#else
        ot::AdiosWriter writer(params, sim.global_nx(), sim.global_ny(), sim.local_ny(),
                               sim.y_offset(), sim.rank());
#endif

        if (rank == 0)
        {
            std::cout << std::setprecision(6) << std::fixed;
            std::cout << "Running Orszag-Tang MHD\n"
                      << "  Grid: " << params.nx << " x " << params.ny << "\n"
                      << "  Solver: " << ot::reconstruction_to_string(params.reconstruction) << "+"
                      << ot::flux_to_string(params.flux) << "\n"
                      << "  tfinal: " << params.tfinal << ", CFL: " << params.cfl << "\n"
                      << "  Output: " << params.output_file << " (" << params.adios_engine << ")\n";
            if (!params.prepend_var_names.empty())
            {
                std::cout << "  Variable prefix: " << params.prepend_var_names << "\n";
            }
            if (params.fixed_dt > 0.0)
            {
                std::cout << "  Fixed dt: " << params.fixed_dt << "\n";
            }
            if (params.output_psi || params.output_m || params.output_E)
            {
                std::cout << "  Extra outputs:";
                if (params.output_psi)
                {
                    std::cout << " psi";
                }
                if (params.output_m)
                {
                    std::cout << " mx,my,mz";
                }
                if (params.output_E)
                {
                    std::cout << " E";
                }
                std::cout << "\n";
            }
        }

        double t = 0.0;
        int step = 0;

        if (params.save_initial)
        {
            ot::OutputFields fields;
            ot::ScalarDiagnostics diagnostics;
            sim.extract_output(fields);
            sim.compute_diagnostics(diagnostics);
            writer.write(step, t, fields, diagnostics);
        }

        const double no_time_cadence = std::numeric_limits<double>::infinity();
        double next_time_dump =
            params.output_every_time > 0.0 ? params.output_every_time : no_time_cadence;

        while (t < params.tfinal && step < params.max_steps)
        {
            double dt = sim.compute_time_step();
            if (!std::isfinite(dt) || dt <= 0.0)
            {
                throw std::runtime_error("Non-finite or non-positive dt computed");
            }
            if (t + dt > params.tfinal)
            {
                dt = params.tfinal - t;
            }

            sim.advance(dt);
            t += dt;
            ++step;

            const bool due_step =
                (params.output_every_steps > 0) && (step % params.output_every_steps == 0);
            bool due_time = false;
            if (params.output_every_time > 0.0 && t + 1.0e-14 >= next_time_dump)
            {
                due_time = true;
                while (t + 1.0e-14 >= next_time_dump)
                {
                    next_time_dump += params.output_every_time;
                }
            }

            const bool final_dump = (t + 1.0e-14 >= params.tfinal);
            if (due_step || due_time || final_dump)
            {
                ot::OutputFields fields;
                ot::ScalarDiagnostics diagnostics;
                sim.extract_output(fields);
                sim.compute_diagnostics(diagnostics);
                writer.write(step, t, fields, diagnostics);
            }

            if (rank == 0 && params.log_every_steps > 0 && step % params.log_every_steps == 0)
            {
                std::cout << "step=" << step << " t=" << t << " dt=" << dt << "\n";
            }
        }

        if (rank == 0)
        {
            std::cout << "Completed: steps=" << step << " t=" << t << "\n";
            if (step >= params.max_steps && t < params.tfinal)
            {
                std::cout << "Stopped at max_steps before tfinal\n";
            }
        }

        writer.close();

#ifdef OT_USE_MPI
        MPI_Finalize();
#endif
        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << "\n";
#ifdef OT_USE_MPI
        MPI_Finalize();
#endif
        return 1;
    }
}
