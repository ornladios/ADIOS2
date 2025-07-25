import numpy
import adios2

#
# Create many files for a campaign, each with a single small array
#

NFILES = 100

# User data
myArray = numpy.array([0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0])
Nx = myArray.size

for n in range(NFILES):
    myArray += 1.0
    with adios2.Stream(f"data{n:03}.bp", "w") as fh:
        fh.write("d", myArray, [Nx], [0], [Nx])
