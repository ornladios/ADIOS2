# test_hello.py
#  Created on: Feb 2, 2017
#      Author: wfg

from mpi4py import MPI
from ADIOSPy import *
import numpy as np

# Create ADIOS and verify MPI Comm is passed correctly
adios = ADIOSPy( MPI.COMM_WORLD, True ) #Pass communicator and debug flag is True
rank = MPI.COMM_WORLD.Get_rank()
size = MPI.COMM_WORLD.Get_size()

# User data
myArray = np.array( [1,2,3,4], dtype=np.double )

if( rank % 2 == 1 ):  # odd ranks only
    oddRankArray = np.array([11,12,13,14],dtype=np.float)

# ADIOS Define Variables    
ioMyDoubles = adios.DefineVariableDouble( "ioMyDoubles", [myArray.size], [], [] )

if( rank % 2 == 1 ): # odd ranks only
    ioMyFloats = adios.DefineVariableDouble( "ioMyFloats", [oddRankArray.size], [], [] )

#Setup method and print summary
ioSettings = adios.DeclareMethod("adiosSettings", "BPFileWriter")
ioSettings.SetParameters( max_buffer_size = '10000000' )
ioSettings.AddTransport( 'File', have_metadata_file = 'yes', library = 'POSIX' )  # POSIX is default, just checking

#Start Engine
bpFileWriter = adios.Open( "file.bp", "w", ioSettings, None )  # Open files using N-to-N method, None means no new MPI communicator
bpFileWriter.Write( ioMyDoubles, myArray )

if( rank % 2 == 1 ): 
    bpFileWriter.Write( ioMyFloats, oddRankArray )

bpFileWriter.Close( ) 

if( rank == 0 ):
    print "Done writing " + str( size ) + " bp files"
    ioSettings.PrintAll( ) # just prints a summary of Method/Transport parameters
    