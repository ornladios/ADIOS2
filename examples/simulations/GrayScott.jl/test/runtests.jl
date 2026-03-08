# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

import MPI

# Run all lightweight unit tests within a single MPI session
MPI.Init()

verbose = false

# unit tests for module GrayScott 
include(joinpath("unit", "helper", "unit-helperMPI.jl"))

include(joinpath("unit", "simulation", "unit-Inputs.jl"))
include(joinpath("unit", "simulation", "unit-Simulation.jl"))
include(joinpath("unit", "simulation", "unit-Simulation_CUDA.jl"))
include(joinpath("unit", "simulation", "unit-IO.jl"))

# unit tests for analysis scripts
include(joinpath("unit", "analysis", "unit-pdfcalc.jl"))

MPI.Finalize()

# Command line tests. These are heavier tests launched as separate processes.
# The downside is that only global success can be tested and not internal states.

# functional tests
# include(joinpath("functional", "functional-GrayScott.jl"))
