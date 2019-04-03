#!/usr/bin/env python

#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestChangingShape.py
#
#  Created on: April 2nd, 2019
#      Author: Jeremy Logan

import numpy as np
from mpi4py import MPI
import adios2

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# Test data
nx = [10, 15]
data = [np.zeros(nx[0]), np.ones(nx[1])]
shape = [[size * nx[0]], [size * nx[1]]]
start = [[rank * nx[0]], [rank * nx[1]]]
count = [[nx[0]], [nx[1]]]

# Write different sized arrays as separate steps
with adios2.open('out.bp', 'w', comm) as f:
    f.write('z', data[0], shape[0], start[0], count[0], end_step=True)
    f.write('z', data[1], shape[1], start[1], count[1], end_step=True)

# Read back arrays
with adios2.open('out.bp', 'r', comm) as f:
    for f_step in f:
        shape_z = int(f_step.available_variables()['z']['Shape'])
        print(shape_z)
        assert(shape_z == int(shape[f_step.current_step()][0]))
