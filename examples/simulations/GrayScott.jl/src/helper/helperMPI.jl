# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

import MPI

export bcase_file

function bcast_file_contents(file_name::String, comm, root = 0)::String
    size::UInt32 = 0
    data::Vector{UInt8} = []
    if MPI.Comm_rank(comm) == root
        data = read(open(file_name, "r"))
    end

    data = MPI.bcast(data, comm)
    return String(data)
end
