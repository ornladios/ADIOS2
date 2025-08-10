import numpy
import adios2

#
# Create many files for a campaign, each with a single small array
#

NFILES = 100

# User data
myArray = numpy.arange(0.0, 9.0, 1)
Nx = myArray.size

for n in range(NFILES):
    myArray += 1.0
    with adios2.Stream(f"data{n:03}.bp", "w") as fh:
        fh.write("d", myArray, [Nx], [0], [Nx])
