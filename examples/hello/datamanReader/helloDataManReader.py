#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# helloDataManReader.py
#
#  Created on: Sept 5, 2019
#      Author: Jason Wang <wangr1@ornl.gov>
#

from mpi4py import MPI
import numpy as np
import adios2

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

adios = adios2.ADIOS(comm)
io = adios.DeclareIO("whatever")
io.SetEngine("DataMan")
io.SetParameters({"IPAddress": "127.0.0.1", "Port": "12306", "Timeout": "5"})

engine = io.Open('HelloDataMan', adios2.Mode.Read, comm)

while True:
    stepStatus = engine.BeginStep()
    if stepStatus == adios2.StepStatus.OK:
        var = io.InquireVariable("FloatArray")
        shape = var.Shape()
        floatArray = np.zeros(np.prod(shape), dtype=np.float32)
        engine.Get(var, floatArray, adios2.Mode.Sync)
        currentStep = engine.CurrentStep()
        engine.EndStep()
        print("Step", currentStep, floatArray)
    elif stepStatus == adios2.StepStatus.EndOfStream:
        print("End of stream")
        break

engine.Close()
