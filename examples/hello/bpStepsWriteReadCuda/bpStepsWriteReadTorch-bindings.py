import numpy as np
import torch
from adios2 import FileReader
import adios2.bindings as adios2

def write_array(fileName, nSteps, gpuArray, cpuArray):
    adios = adios2.ADIOS()
    ioWriter = adios.DeclareIO("torchWriter")
    varType = str(gpuArray.dtype).split('.')[-1]
    gpuVar = ioWriter.DefineVariable("gpuArray", np.array([0], dtype=varType),
                                     gpuArray.shape, [0] * len(gpuArray.shape),
                                     gpuArray.shape)
    # optionally the memory space can be set to GPU
    gpuVar.SetMemorySpace(adios2.MemorySpace.GPU)
    cpuVar = ioWriter.DefineVariable("cpuArray", cpuArray, cpuArray.shape,
                                     [0] * len(cpuArray.shape), cpuArray.shape)

    # write both cpu and gpu arrays for each simulation step
    wStream = ioWriter.Open(fileName, adios2.Mode.Write)
    for step in range(nSteps):
        # write buffers
        wStream.BeginStep()
        wStream.Put(cpuVar, cpuArray)
        wStream.Put(gpuVar, gpuArray.data_ptr(), adios2.Mode.Deferred)
        wStream.EndStep()
        # update buffers
        gpuArray = gpuArray * 2
        cpuArray = cpuArray + 1
    wStream.Close()
    print("Write to file %s: %s data from GPU and %s data from CPU" % (
        fileName, gpuArray.shape, cpuArray.shape))

def read_array(fileName, nSteps):
    adios = adios2.ADIOS()
    ioReader = adios.DeclareIO("torchReader")
    rStream = ioReader.Open(fileName, adios2.Mode.Read)
    for step in range(nSteps):
        rStream.BeginStep()
        # prepare input buffers
        gpuVar = ioReader.InquireVariable("gpuArray")
        cpuVar = ioReader.InquireVariable("cpuArray")
        cpuBuffer = np.zeros(cpuVar.Shape(), dtype=np.float32)
        gpuShape = gpuVar.Shape(adios2.MemorySpace.GPU)
        cuda0 = torch.device('cuda:0')
        gpuBuffer = torch.zeros(gpuShape, dtype=torch.float32, device=cuda0)
        gpuVar.SetSelection([(0, 0), gpuShape])
        # populate data
        rStream.Get(gpuVar, gpuBuffer.data_ptr(), adios2.Mode.Deferred)
        rStream.Get(cpuVar, cpuBuffer)
        rStream.EndStep()
        print("Step %d: read GPU data\n %s" % (step, gpuBuffer))
        print("Step %d: read CPU data\n %s" % (step, cpuBuffer))
    rStream.Close()


if __name__ == '__main__':
    cpuArray = np.array([[0, 1.0, 2.0], [3.0, 4.0, 5.0]], dtype=np.float32)
    cuda0 = torch.device('cuda:0')
    gpuArray = torch.tensor([[0, 1.0, 2.0], [3.0, 4.0, 5.0]],
                            dtype=torch.float32, device=cuda0)
    print("Array allocation: ", gpuArray.device)
    print("Bytes required to store the gpu array", gpuArray.nbytes)

    nSteps = 2
    write_array("StepsTorchBindings.bp", nSteps, gpuArray, cpuArray)
    read_array("StepsTorchBindings.bp", nSteps)
