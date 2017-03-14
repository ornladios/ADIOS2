# test_hello.py
#  Created on: Feb 2, 2017
#      Author: wfg

from mpi4py import MPI
from ADIOSPy import *

adios = ADIOSPy( MPI.COMM_WORLD, True)
adios.HelloMPI( )

rank = MPI.COMM_WORLD.Get_rank()
lDims = [rank+1, rank+2, rank+3]
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
    
method = adios.DeclareMethod("myMethod", "BPFileWriter")
method.SetParameters( max_buffer_size = '10000000' )
method.AddTransport( 'File', have_metadata_file = 'yes', library = 'FStream' )
method.AddTransport( "Mdtm", localIP='128.0.0.0.1', remoteIP='128.0.0.0.2', tolerances='1,2,3' )
print
method.PrintAll( )



# bpWriter = adios.Open( )
# ADIOS.SetEngineComm( bpWriter, comm ) 
# bpWriter.Hello( )

# challenge is to pass comm to C++
