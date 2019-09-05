#!/usr/bin/env python

#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestBPWriteTypes.py: test Python numpy types in ADIOS2 File
#                      Write/Read High-Level API
#  Created on: March 12, 2018
#      Author: William F Godoy godoywf@ornl.gov

import sys
from mpi4py import MPI
import numpy as np
import adios2
#import pdb

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# Test data
nx = 10

shape = [size * nx]
start = [rank * nx]
count = [nx]


# Reader

with adios2.open("types_np.h5", "r", comm, "HDF5") as fr:
    
#    pdb.set_trace()
    for fr_step in fr:

        step = fr_step.currentstep()
        
        print("Reader Step: " + str(step))

        step_vars = fr_step.availablevariables()

#        for name, info in step_vars.items():
#            print("variable_name: " + name)
#            for key, value in info.items():
#                print("\t" + key + ": " + value)
#            print("\n")

        indataR64 = fr_step.read("varR64", start, count)
        print("\tvarR64: " + str(indataR64))


#        stepStr = "Step:" + str(step)
#
#        instepStr = fr_step.readstring("steps")
#        print("\tsteps: " + str(instepStr))
#        if(instepStr != stepStr):
#            raise ValueError('steps variable read failed: ' +
#                             instepStr + " " + stepStr)



