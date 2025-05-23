#!/usr/bin/env python

from adios2NPTypes import SmallTestData
from mpi4py import MPI
import numpy as np

try:
    import cupy as cp

    ADIOS2_HAS_CUPY = True
except ImportError:
    ADIOS2_HAS_CUPY = False
try:
    import torch

    ADIOS2_HAS_TORCH = True
except ImportError:
    ADIOS2_HAS_TORCH = False

from adios2 import Stream, LocalValueDim

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# Test data
data = SmallTestData(rank)
nx = data.Nx

shape = [size * nx]
start = [rank * nx]
count = [nx]

# Writer
with Stream("arrays_np.bp", "w", comm=comm) as s:
    for step in s.steps(5):
        data.update(rank, step.current_step(), size)
        s.write("varNumpy", data.u16, shape, start, count)
        if ADIOS2_HAS_CUPY:
            gpuArray = cp.asarray(data.u16)
            s.write("varCupy", gpuArray, shape, start, count)
        if ADIOS2_HAS_TORCH:
            tensor = torch.tensor(data.u16)
            s.write("varTensor", tensor, shape, start, count)

comm.Barrier()

# Reader
data = SmallTestData(rank)

with Stream("arrays_np.bp", "r", comm=comm) as fr:
    # file only
    assert fr.num_steps() == 5

    for fr_step in fr.steps():
        step = fr_step.current_step()
        data.update(rank, step, size)

        step_vars = fr_step.available_variables()
        indataU16 = np.zeros(count, dtype=data.u16.dtype)
        fr_step.read_in_buffer("varNumpy", indataU16, start, count)
        if not (indataU16 == data.u16).all():
            raise ValueError("u16 numpy array read failed")

        if ADIOS2_HAS_CUPY:
            cupyU16 = cp.zeros(count, dtype=data.u16.dtype)
            fr_step.read_in_buffer("varCupy", cupyU16, start, count)
            indataU16 = np.array(cupyU16)
            if not (indataU16 == data.u16).all():
                raise ValueError("u16 cupy array read failed")

        if ADIOS2_HAS_TORCH:
            torchU16 = torch.zeros(count, dtype=torch.uint16)
            fr_step.read_in_buffer("varTensor", torchU16, start, count)
            indataU16 = torchU16.numpy()
            if not (indataU16 == data.u16).all():
                raise ValueError("u16 torch tensor read failed")
