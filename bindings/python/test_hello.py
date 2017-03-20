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
myArray = np.array( [1,2,3,4])

if( rank % 2 == 1 ):  # odd ranks only
    oddRankArray = np.array( [11.,12.,13.,14.] )
    

# ADIOS Define Variables    
ioArray = adios.DefineVariable( "ioArray", [myArray.size], [], [] )

if( rank % 2 == 1 ): # odd ranks only
    ioOddRankArray = adios.DefineVariable( "ioMyFloats", [oddRankArray.size], [], [] )
    

#Setup method and print summary
ioSettings = adios.DeclareMethod("adiosSettings", "BPFileWriter")
ioSettings.SetParameters( max_buffer_size = '10000000' )
ioSettings.AddTransport( 'File', have_metadata_file = 'yes', library = 'POSIX' )  # POSIX is default, just checking

#Start Engine
bpFileWriter = adios.Open( "file.bp", "w", ioSettings, None )  # Open files using N-to-N method, None means no new MPI communicator
bpFileWriter.Write( ioArray, myArray )

if( rank % 2 == 1 ): 
    bpFileWriter.Write( ioOddRankArray, oddRankArray )
    
bpFileWriter.Close( ) 

if( rank == 0 ):
    print "Done writing " + str( size ) + " bp files"
    ioSettings.PrintAll( ) # just prints a summary of Method/Transport parameters