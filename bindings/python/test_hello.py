# test_hello.py
#  Created on: Feb 2, 2017
#      Author: wfg

from mpi4py import MPI
from ADIOSPy import *

adios = ADIOSPy( MPI.COMM_WORLD, True)
adios.HelloMPI( )

lDims = [10, 11, 12]
Nx = 1

ioMyDoubles = adios.DefineVariableDouble( "ioMyDoubles", lDims, [], [] )

dims = ioMyDoubles.GetLocalDimensions( )
print "Old Dimensions" 
for dim in dims:
    print dim

ioMyDoubles.SetLocalDimensions( [20,20,20] )

dims = ioMyDoubles.GetLocalDimensions( )
print "New Dimensions " 
for dim in dims:
    print dim


# dims = adios.GetVariableLocalDimensions( ioMyDoubles )
# 
# lDims = [20,20,20]
# adios.SetVariableLocalDimensions( ioMyDoubles, lDims )
# 
# dims = adios.GetVariableLocalDimensions( ioMyDoubles )
# print "New Dimensions " 
# for dim in dims:
#     print dim


# bpWriter = adios.Open( )
# ADIOS.SetEngineComm( bpWriter, comm ) 
# bpWriter.Hello( )

# challenge is to pass comm to C++
