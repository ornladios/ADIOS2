#!/usr/bin/env python

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

import os

from adios2NPTypes import SmallTestData
from mpi4py import MPI
import numpy as np

try:
    import cupy as cp

    ADIOS2_HAS_CUPY = cp.cuda.is_available()
except ImportError:
    ADIOS2_HAS_CUPY = False
try:
    import torch

    ADIOS2_HAS_TORCH = True
    torch_device = "cuda" if torch.cuda.is_available() else "cpu"
except ImportError:
    ADIOS2_HAS_TORCH = False

# Without a GPU the test uses what it finds.  ADIOS2_TEST_EXPECT_CUDA makes it
# fail instead when cupy or torch cannot use a CUDA device.
if os.environ.get("ADIOS2_TEST_EXPECT_CUDA"):
    if not ADIOS2_HAS_CUPY:
        raise RuntimeError("ADIOS2_TEST_EXPECT_CUDA is set but cupy has no CUDA device")
    if not ADIOS2_HAS_TORCH or torch_device != "cuda":
        raise RuntimeError("ADIOS2_TEST_EXPECT_CUDA is set but torch has no CUDA device")

from adios2 import Stream, LocalValueDim
from adios2.bindings import ArrayOrdering

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
            tensor = torch.tensor(data.u16, device=torch_device)
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
            indataU16 = cp.asnumpy(cupyU16)
            if not (indataU16 == data.u16).all():
                raise ValueError("u16 cupy array read failed")

        if ADIOS2_HAS_TORCH:
            torchU16 = torch.zeros(count, dtype=torch.uint16, device=torch_device)
            fr_step.read_in_buffer("varTensor", torchU16, start, count)
            indataU16 = torchU16.cpu().numpy()
            if not (indataU16 == data.u16).all():
                raise ValueError("u16 torch tensor read failed")

# A multidimensional GPU buffer needs an explicit array layout: without one
# Put and Get throw, RowMajor keeps the declared shape, ColumnMajor reverses it.
gpu2d = []
if ADIOS2_HAS_CUPY:
    gpu2d.append(("Cupy", cp.asarray, cp.zeros, cp.asnumpy))
if ADIOS2_HAS_TORCH and torch_device == "cuda":
    gpu2d.append(
        (
            "Tensor",
            lambda a: torch.tensor(a, device="cuda"),
            lambda n, dtype: torch.zeros(n, dtype=torch.float32, device="cuda"),
            lambda t: t.cpu().numpy(),
        )
    )

if gpu2d:
    host2d = np.arange(6, dtype=np.float32).reshape(2, 3) + 6 * rank
    shape2d = [2 * size, 3]
    start2d = [2 * rank, 0]
    count2d = [2, 3]

    with Stream("arrays_2d.bp", "w", comm=comm) as s:
        for _ in s.steps(1):
            for name, to_device, _zeros, _to_host in gpu2d:
                device2d = to_device(host2d)
                var = s.io.define_variable(name + "NoLayout", host2d, shape2d, start2d, count2d)
                try:
                    s.write(var, device2d)
                    raise RuntimeError(name + ": 2-D GPU write without a layout did not throw")
                except ValueError:
                    pass
                var = s.io.define_variable(name + "Row", host2d, shape2d, start2d, count2d)
                var.set_array_layout(ArrayOrdering.RowMajor)
                s.write(var, device2d)
                var = s.io.define_variable(name + "Col", host2d, shape2d, start2d, count2d)
                var.set_array_layout(ArrayOrdering.ColumnMajor)
                s.write(var, device2d)

    comm.Barrier()

    with Stream("arrays_2d.bp", "r", comm=comm) as fr:
        for _ in fr.steps():
            for name, _to_device, zeros, to_host in gpu2d:
                if fr.inquire_variable(name + "NoLayout") is not None:
                    raise ValueError(name + ": rejected 2-D GPU write stored a variable")
                if fr.inquire_variable(name + "Col").shape() != [3, 2 * size]:
                    raise ValueError(name + ": ColumnMajor 2-D GPU write not stored reversed")

                var = fr.inquire_variable(name + "Row")
                if var.shape() != shape2d:
                    raise ValueError(name + ": RowMajor 2-D GPU write changed the shape")
                buffer = zeros(6, np.float32)
                try:
                    fr.read_in_buffer(var, buffer, start2d, count2d)
                    raise RuntimeError(name + ": 2-D GPU read without a layout did not throw")
                except ValueError:
                    pass

    # read with the layout set in a new stream, so no earlier Get touched the variables
    with Stream("arrays_2d.bp", "r", comm=comm) as fr:
        for _ in fr.steps():
            for name, _to_device, zeros, to_host in gpu2d:
                var = fr.inquire_variable(name + "Row")
                var.set_array_layout(ArrayOrdering.RowMajor)
                buffer = zeros(6, np.float32)
                fr.read_in_buffer(var, buffer, start2d, count2d)
                if not (to_host(buffer).reshape(2, 3) == host2d).all():
                    raise ValueError(name + ": RowMajor 2-D GPU read returned wrong data")
