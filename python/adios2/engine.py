"""License:
Distributed under the OSI-approved Apache License, Version 2.0.  See
accompanying file Copyright.txt for details.
"""

import numpy as np

# pylint: disable=duplicate-code
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
# pylint: enable=duplicate-code

from adios2 import bindings


class Engine:
    """High level representation of the Engine class in the adios2.bindings"""

    def __init__(self, implementation):
        self.impl = implementation

    @property
    def impl(self):
        """Bindings implementation of the class"""
        return self._impl

    @impl.setter
    def impl(self, implementation):
        self._impl = implementation

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def close(self, transport_index=-1):
        """
        Close an transports of the engine

        Args:
            transportIndex (int): if -1 close all transports
        """
        self.impl.Close(transport_index)

    def steps(self):
        """Returns number of available steps"""
        return self.impl.Steps()

    def current_step(self):
        """Returns the current step"""
        return self.impl.CurrentStep()

    def begin_step(self, *args, **kwargs):
        """Start step"""
        return self.impl.BeginStep(*args, **kwargs)

    def end_step(self):
        """End step"""
        self.impl.EndStep()

    def between_step_pairs(self):
        """End step"""
        return self.impl.BetweenStepPairs()

    def all_blocks_info(self, name):
        """
        Returns a list of BlocksInfo for a all steps

        Args:
            name (str): variable name

        Returns:
            list of BlockInfos
        """
        output = []
        for step in range(0, self.steps()):
            output.append(self.blocks_info(name, step))
        return output

    def blocks_info(self, name, step):
        """
        Returns a BlocksInfo for a given step

        Args:
            name (str): variable name
            step (int): step in question

        Returns:
            list of dicts describing each BlockInfo
        """
        return self.impl.BlocksInfo(name, step)

    def put(self, variable, content, mode=bindings.Mode.Deferred):
        """
        Puts content in a variable

        Parameters
            name
                variable name

            content
                variable data values

            mode
                when to perform the communication, (Deferred or Asynchronous).
        """
        if isinstance(content, np.ndarray):
            self.impl.Put(variable.impl, content, mode)
        elif isinstance(content, str):
            self.impl.Put(variable.impl, content)
        elif isinstance(content, list):
            self.impl.Put(variable.impl, content)
        elif not hasattr(content, "__len__"):
            content = np.array([content])
            self.impl.Put(variable.impl, content)
        else:
            if ADIOS2_HAS_CUPY:
                if isinstance(content, cp.ndarray):
                    self.impl.Put(variable.impl, content.data.ptr, mode)
                    return
            if ADIOS2_HAS_TORCH:
                if isinstance(content, torch.Tensor):
                    self.impl.Put(variable.impl, content.data_ptr(), mode)
                    return
            raise ValueError

    def perform_puts(self):
        """Perform the puts calls"""
        self.impl.PerformPuts()

    def perform_data_write(self):
        """Perform the transport data writes"""
        self.impl.PerformDataWrite()

    def get(self, variable, content=None, mode=bindings.Mode.Sync):
        """
        Gets the content of a variable

        Parameters
            name
                variable name

            content
                output variable data values

            mode
                when to perform the communication, (Deferred or Asynchronous).
        Returns
            Content of the variable when the content argument is omitted.
        """
        if isinstance(content, np.ndarray):
            self.impl.Get(variable.impl, content, mode)
            return None

        if ADIOS2_HAS_CUPY:
            if isinstance(content, cp.ndarray):
                self.impl.Get(variable.impl, content.data.ptr, mode)
                return None
        if ADIOS2_HAS_TORCH:
            if isinstance(content, torch.Tensor):
                self.impl.Get(variable.impl, content.data_ptr(), mode)
                return None
        return self.impl.Get(variable.impl, mode)

    def perform_gets(self):
        """Perform the gets calls"""
        self.impl.PerformGets()

    def lock_reader_selections(self):
        """Locks the data selection for read"""
        self.impl.LockReaderSelections()

    def lock_writer_definitions(self):
        """Locks the data selection for write"""
        self.impl.LockWriterDefinitions()

    def flush(self, transport_index=-1):
        """Flush all transports attached to this Engine instance"""
        self.impl.Flush(transport_index)

    def get_metadata(self):
        """
        Get metadata of an open file in a serialized form that can be
        sent to other processes and used to open the file again by
        avoiding reading the metadata from disk.
        """
        return self.impl.GetMetadata()
