import numpy as np
import torch
from adios2 import Stream, FileReader

def write_array(fileName, nSteps, gpuArray, cpuArray):
    with Stream(fileName, "w") as wStream:
        for _ in wStream.steps(nSteps):
            wStream.write("cpuArray", cpuArray, cpuArray.shape,
                          [0] * len(cpuArray.shape), cpuArray.shape)
            wStream.write("gpuArray", gpuArray, gpuArray.shape,
                          [0] * len(gpuArray.shape), gpuArray.shape)
            # update buffers
            gpuArray = gpuArray * 2
            cpuArray = cpuArray + 1
    print("Write to file %s: %s data from GPU and %s data from CPU" % (
        fileName, gpuArray.shape, cpuArray.shape))

def read_array(fileName, readGpuShape, readCpuShape):
    with Stream(fileName, "r") as rStream:
        for _ in rStream.steps():
            step = rStream.current_step()
            cpuBuffer = np.zeros(readCpuShape, dtype=np.float32)
            rStream.read_in_buffer("cpuArray", cpuBuffer)

            cuda0 = torch.device('cuda:0')
            gpuBuffer = torch.zeros(readGpuShape, dtype=torch.float32, device=cuda0)
            rStream.read_in_buffer("gpuArray", gpuBuffer)

            print("Step %d: read GPU data\n %s" % (step, gpuBuffer))
            print("Step %d: read CPU data\n %s" % (step, cpuBuffer))


if __name__ == '__main__':
    cpuArray = np.array([[0, 1.0, 2.0], [3.0, 4.0, 5.0]], dtype=np.float32)
    cuda0 = torch.device('cuda:0')
    gpuArray = torch.tensor([[0, 1.0, 2.0], [3.0, 4.0, 5.0]],
                            dtype=torch.float32, device=cuda0)
    print("Array allocation: ", gpuArray.device)
    print("Bytes required to store the gpu array", gpuArray.nbytes)

    nSteps = 2
    write_array("StepsWriteReadTorch.bp", nSteps, gpuArray, cpuArray)
    read_array("StepsWriteReadTorch.bp", gpuArray.shape, cpuArray.shape)
