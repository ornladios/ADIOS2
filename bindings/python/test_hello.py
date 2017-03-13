# test_hello.py
#  Created on: Feb 2, 2017
#      Author: wfg

from mpi4py import MPI
from ADIOSPy import *

adios = ADIOSPy( MPI.COMM_WORLD, True)
adios.HelloMPI( )

lDims = [10, 11, 12]
Nx = 1

print lDims
print lDims[0]
ioMyDoubles = adios.DefineVariableDouble( "ioMyDoubles", lDims )
ioMyFloats = adios.DefineVariableFloat( "ioMyFloats", [Nx] )

print "My ADIOS Variable Double " + ioMyDoubles
print "My ADIOS Variable Float " + ioMyFloats

dims = adios.GetVariableLocalDimensions( ioMyDoubles )
print "Old Dimensions " 
print dims

lDims = [20,20,20]
adios.SetVariableLocalDimensions( ioMyDoubles, lDims )

dims = adios.GetVariableLocalDimensions( ioMyDoubles )
print "New Dimensions " 
print dims


# bpWriter = adios.Open( )
# ADIOS.SetEngineComm( bpWriter, comm ) 
# bpWriter.Hello( )

# challenge is to pass comm to C++
