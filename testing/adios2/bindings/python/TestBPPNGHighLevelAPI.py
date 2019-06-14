#!/usr/bin/env python

#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestBPPNGHighLevelAPI.py
#
#  Created on: June 7th, 2019
#      Author: William F Godoy

import numpy as np
import random
from mpi4py import MPI
import adios2


def CompressPNG(compression_level):

    fname = "BPWRPNG_" + str(compression_level) + "_py.bp"
    Nx = 100
    Ny = 50
    channels = 3
    NSteps = 1

    # initialize values
    u32s = np.zeros([Nx, Ny], np.uint32)
    u8s = np.zeros([Nx, Ny, channels], np.uint8)

    value_ji = 0.
    for i in range(0, Nx):
        for j in range(0, Ny):
            u32s[i][j] = value_ji
            u8s[i][j][0] = random.randrange(256)
            u8s[i][j][1] = random.randrange(256)
            u8s[i][j][2] = random.randrange(256)

            value_ji += 1.

    # set global dimensions
    # MPI
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()

    shape = [Nx * size, Ny]
    start = [Nx * rank, 0]
    count = [Nx, Ny]

    # writer
    with adios2.open(fname, "w", comm) as fw:

        for s in range(0, NSteps):
            fw.write("u8", u8s, shape, start, count,
                     [('PNG', {'bit_depth': '8',
                               'color_type': 'PNG_COLOR_TYPE_RGB',
                               'compression_level': str(compression_level)})])
            fw.write("u32", u32s, shape, start, count,
                     [('PNG', {'bit_depth': '8',
                               'color_type': 'PNG_COLOR_TYPE_RGBA',
                               'compression_level': str(compression_level)})],
                     end_step=True)

    # reader
    with adios2.open(fname, "r", comm) as fr:

        for fstep in fr:

            in_u32s = fstep.read("u32", start, count)
            in_u8s = fstep.read("u8", start, count)

            for i in range(0, Nx):
                for j in range(0, Ny):
                    assert(u32s[i][j] == in_u32s[i][j])
                    assert(u8s[i][j][0] == in_u8s[i][j][0])
                    assert(u8s[i][j][1] == in_u8s[i][j][1])
                    assert(u8s[i][j][2] == in_u8s[i][j][2])


def main():

    CompressPNG(compression_level=1)
    CompressPNG(compression_level=4)
    CompressPNG(compression_level=9)
#     CompressPNG2D(rate=9)
#     CompressPNG2D(rate=10)


if __name__ == "__main__":
    main()
