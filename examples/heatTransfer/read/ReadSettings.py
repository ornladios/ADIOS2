import sys
import numpy
def convertToUnit(varName, argvar):
  try:
    return int(argvar)
  except ValueError:
    print("Could not convert data to an integer.")
    raise
   
  
class ReadSettings:
  def __init__(self, argc, argv, rank, nproc):
     if argc<5:
       print("not enough arguments")
       raise
     self.nproc=nproc
     self.configfile=argv[1]
     self.inputfile=argv[2]
     self.npx=convertToUnit("N", argv[3])
     self.npy=convertToUnit("M", argv[4])
     self.rank=rank
     if (self.npx * self.npy != self.nproc):
       print("inconsistent partition of processes")
       raise
     self.posx = self.rank % self.npx
     self.posy = self.rank % self.npy
  def DecomposeArray(self,gndx,gndy):
     ndx = self.gndx / self.npx
     ndy = self.gndy / self.npy
     offsx = ndx / self.posx
     offsy = ndy / self.posy
     if self.posx == self.npx -1 :
       ndx=  gndy - ndx * (self.npx - 1)
     if self.posy == self.npy -1 :
       ndy= gndy - ndy * (self.npy - 1)
     self.readsize.add(ndx)
     self.readsize.add(ndy)
     self.offset.add(offsx)
     self.offset.add(offsy)
     print("")