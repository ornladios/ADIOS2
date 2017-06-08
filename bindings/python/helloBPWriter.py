#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# test_hello.py
#  Created on: Feb 2, 2017
#      Author: wfg

from mpi4py import MPI
from adios2py import *
import numpy as np


# User data
myArray = np.array([1, 2, 3, 4])

print "Read " + str(Read)

adios = ADIOS(MPI.COMM_WORLD, True)

bpIO = adios.DeclareIO("BPN2N")
ioArray = bpIO.DefineVariable("bpArray", [myArray.size], [0], [myArray.size], True)
  
bpFileWriter = bpIO.Open("myArray.bp", Write)
bpFileWriter.Write(ioArray, myArray)
bpFileWriter.Close()
