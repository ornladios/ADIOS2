import sys
from mpi4py import MPI
import numpy
import adios2

#import PrintDataStep
from ReadSettings import ReadSettings
def  printUsage():

    print("Usage: heatRead  config  input  N  M \n")
    print("  config: XML config file to use\n")
    print("  input:  name of input data file/stream\n")
    print("  N:      number of processes in X dimension\n")
    print("  M:      number of processes in Y dimension\n\n")
comm = MPI.COMM_WORLD
wrank = comm.Get_rank()
wnproc = comm.Get_size()
color=2
mpiReaderComm=comm.Split(color, wrank)
rank = mpiReaderComm.Get_rank()
nproc = mpiReaderComm.Get_size()

try:
  settings= ReadSettings(len(sys.argv), sys.argv, rank, nproc)
  ad = adios2.ADIOS(settings.configfile, mpiReaderComm, adios2.DebugON)
  bpReaderIO = ad.DeclareIO("reader")
  if not bpReaderIO.InConfigFile():
    bpReaderIO.SetEngine("ADIOS1Reader");
    bpReaderIO.SetParameters({{"num_threads", "2"}})
    bpReaderIO.AddTransport("File", {{"verbose", "4"}})
  bpReader = bpReaderIO.Open(settings.inputfile, adios2.Mode.Read, mpiReaderComm)
  while True:
    status = bpReader.BeginStep(adios2.StepMode.NextAvailable, 10.0);
    if status != adios2.StepStatus.OK:
       break
    vT = bpReaderIO.InquireVariable("T");
    if (firstStep):
      gndx = vT.m_Shape[0];
      gndy = vT.m_Shape[1];
      if rank == 0 :
        print("gndx       = " + gndx)
        print("gndy       = " + gndy)
        settings.DecomposeArray(gndx, gndy);
        T = settings.readsize[0] * settings.readsize[1]
        firstStep = False
        MPI.MPI_Barrier(mpiReaderComm)
            

      if not rank:
        print("Processing step " + step)
        vT.SetSelection(adios2.Box<adios2.Dims>(settings.offset, settings.readsize));

        bpReader.GetDeferred(vT, T);
        bpReader.PerformGets();
        printDataStep(T, settings.readsize.data(), settings.offset.data(),
                          rank, step);
      bpReader.EndStep();
      step=step+1;
  bpReader.Close()
except:
    print("Unexpected error")
    raise
    


