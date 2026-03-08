# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

import GrayScott

# using Profile
# using PProf

function julia_main()::Cint
    GrayScott.main(ARGS)
    return 0
end

if !isdefined(Base, :active_repl)
    @time julia_main()
    # @profile julia_main()
    # pprof()
end