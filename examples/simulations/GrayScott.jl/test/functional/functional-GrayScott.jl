# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

import GrayScott
import Test: @testset, @test

@testset "GrayScott" begin MPI.mpiexec() do runcmd
    config_file = joinpath(dirname(Base.active_project()), "examples",
                           "settings-files.json")

    juliacmd = `julia --project gray-scott.jl $config_file`

    @test run(`mpirun -n 4 $juliacmd`).exitcode == 0
end end