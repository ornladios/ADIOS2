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
dataManIO = adios.DeclareIO("whatever")
dataManIO.SetEngine("DataMan")
dataManIO.SetParameters({"IPAddress": "127.0.0.1",
                         "Port": "12306",
                         "Timeout": "5"})

dataManReader = dataManIO.Open('HelloDataMan', adios2.Mode.Read, comm)

while True:
    stepStatus = dataManReader.BeginStep()
    if stepStatus == adios2.StepStatus.OK:
        var = dataManIO.InquireVariable("FloatArray")
        shape = var.Shape()
        floatArray = np.zeros(np.prod(shape), dtype=np.float32)
        dataManReader.Get(var, floatArray, adios2.Mode.Sync)
        currentStep = dataManReader.CurrentStep()
        dataManReader.EndStep()
        print("Step", currentStep, floatArray)
    elif stepStatus == adios2.StepStatus.EndOfStream:
        print("End of stream")
        break

dataManReader.Close()
