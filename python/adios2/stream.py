"""License:
Distributed under the OSI-approved Apache License, Version 2.0.  See
accompanying file Copyright.txt for details.
"""

from functools import singledispatchmethod
from sys import maxsize
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

from adios2 import bindings, Adios, IO, Variable


def type_adios_to_numpy(name):
    """Translation between numpy and adios2 types"""
    return {
        "char": np.int8,
        "int8_t": np.int8,
        "uint8_t": np.uint8,
        "int16_t": np.int16,
        "uint16_t": np.uint16,
        "int32_t": np.int32,
        "uint32_t": np.uint32,
        "int64_t": np.int64,
        "uint64_t": np.uint64,
        "float": np.float32,
        "double": np.float64,
        "complex": np.complex64,
        "double complex": np.complex128,
    }[name]


def string_to_mode(mode: str) -> [bindings.Mode, bool]:
    """Convert high level open mode to adios2.bindings.Mode"""
    read_mode = False
    if mode == "r":
        bmode = bindings.Mode.Read
        read_mode = True
    elif mode == "rra":
        bmode = bindings.Mode.ReadRandomAccess
        read_mode = True
    elif mode == "w":
        bmode = bindings.Mode.Write
    elif mode == "a":
        bmode = bindings.Mode.Append
    else:
        raise ValueError()
    return bmode, read_mode


# pylint: disable=R0902   # Too many instance attributes
class Stream:
    """High level implementation of the Stream class from the core API"""

    # Default timeout for stream.begin_step()
    DEFAULT_TIMEOUT_SEC = -1.0

    @singledispatchmethod
    def __init__(self, path, mode, comm=None):
        # pylint: disable=R0912 # Too many branches
        if comm and not bindings.is_built_with_mpi:
            raise RuntimeError("Cannot use MPI since ADIOS2 was built without MPI support")

        # pylint: disable=E1121
        if comm:
            self._adios = Adios(comm)
        else:
            self._adios = Adios()

        self._io_name = f"stream:{path}:mode:{mode}"

        # pylint: enable=E1121
        self._io = self._adios.declare_io(self._io_name)
        self._mode, self._read_mode = string_to_mode(mode)
        self._engine = self._io.open(path, self._mode)
        self.index = -1
        self.max_steps = maxsize
        self._step_status = bindings.StepStatus.EndOfStream
        self._step_timeout_sec = self.DEFAULT_TIMEOUT_SEC

    # e.g. Stream(io: adios2.IO, path, mode)
    @__init__.register(IO)
    def _(self, io: IO, path, mode, comm=None):
        self._io = io
        self._adios = io.adios()
        self._mode, self._read_mode = string_to_mode(mode)
        self._engine = self._io.open(path, self._mode, comm)
        self.index = -1
        self.max_steps = maxsize
        self._step_status = bindings.StepStatus.EndOfStream
        self._step_timeout_sec = self.DEFAULT_TIMEOUT_SEC

    @property
    def mode(self):
        """Selected open mode"""
        return self._mode

    @property
    def adios(self):
        """Adios instance associated to this Stream"""
        return self._adios

    @property
    def io(self):
        """IO instance associated to this Stream"""
        return self._io

    @property
    def engine(self):
        """Engine instance associated to this Stream"""
        return self._engine

    def __repr__(self):
        return f"<adios.file named {self._io_name} and mode {self._mode}>"

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def __iter__(self):
        return self

    def __next__(self):
        if self.index >= 0 and self._step_status == bindings.StepStatus.OK:
            self.end_step()
        if self.index == self.max_steps - 1:
            self._step_status = bindings.StepStatus.EndOfStream
            raise StopIteration

        self.index += 1
        self._step_status = self.begin_step(timeout=self._step_timeout_sec)
        if self._step_status == bindings.StepStatus.EndOfStream:
            raise StopIteration

        if self._step_status == bindings.StepStatus.NotReady:
            print(
                "ERROR: Stream returned no new step within the time limit of"
                f" {self._step_timeout_sec} seconds. Ending the loop"
            )
            raise StopIteration

        if self._step_status == bindings.StepStatus.OtherError:
            print("ERROR: Stream returned an error. Ending the loop")
            raise StopIteration

        return self

    def get_metadata(self):
        """
        Get metadata of an open file in a serialized form that can be
        sent to other processes and used to open the file again by
        avoiding reading the metadata from disk.
        """
        return self._engine.get_metadata()

    def step_status(self):
        """Inspect the stream status. Return adios2.bindings.StepStatus"""
        return self._step_status

    def set_parameters(self, **kwargs):
        """
        Sets parameters using a dictionary.
        Removes any previous parameter.

        Parameters
            parameters
                input key/value parameters

            value
                parameter value
        """
        self._io.set_parameters(**kwargs)

    def available_variables(self):
        """
        Returns a 2-level dictionary with variable information.
        Read mode only.

        Returns
            variables dictionary
                key
                    variable name
                value
                    variable information dictionary
        """
        return self._io.available_variables()

    def available_attributes(self, varname="", separator="/"):
        """
        Returns a 2-level dictionary with attribute information.
        Read mode only.

        Parameters
            variable_name
                If varname is set, attributes assigned to that variable are returned.
                The keys returned are attribute names with the prefix of varname + separator
                removed.

            separator
                concatenation string between variable_name and attribute
                e.g. varname + separator + name ("var/attr")
                Not used if varname is empty

        Returns
            attributes dictionary
                key
                    attribute name
                value
                    attribute information dictionary
        """
        return self._io.available_attributes(varname, separator)

    def define_variable(self, name):
        """
        Define new variable without specifying its type and content.
        This only works for string output variables
        """
        return self._io.define_variable(name)

    def inquire_variable(self, name):
        """
        Inquire a variable

        Parameters
            name
                variable name
        Returns
            The variable if it is defined, otherwise None
        """
        return self._io.inquire_variable(name)

    def inquire_attribute(self, name, variable_name="", separator="/"):
        """
        Inquire an attribute

        Parameters
            name
                attribute name

            variable_name
                if attribute is associated with a variable

            separator
                concatenation string between variable_name and attribute
                e.g. variable_name + separator + name ("var/attr")
                Not used if variable_name is empty

        Returns
            The attribute if it is defined, otherwise None
        """
        return self._io.inquire_attribute(name, variable_name, separator)

    @singledispatchmethod
    def write(self, variable: Variable, content):
        """
        Writes a variable.
        Note that the content will be available for consumption only at
        the end of the for loop or in the end_step() call.

        Parameters
            variable
                adios2.Variable object to be written
                Use variable.set_selection(), set_shape(), add_operation_string()
                to prepare a write

            content
                variable data values
        """
        self._engine.put(variable, content, bindings.Mode.Sync)

    def _get_variable_selection(self, content, shape, start, count):
        """
        Internal function to get the shape, start and count when defining a variable for write.

        Parameters
           content
                The array that will be written to a variable
            shape
                The shape requested for the write
            start
                The start requested for the write
            count
                The count requested for the write
        Returns
            shapt, start, count
                Either what was selected or dimensions given by the content
        """
        if shape == [] and start == [] and count == []:
            shape = list(content.shape)
            start = [0] * content.ndim
            count = shape[:]
        return shape, start, count

    @write.register(str)
    def _(self, name, content, shape=[], start=[], count=[], operations=None):
        """
        Writes a variable

        Parameters
            name
                variable name

            content
                variable data values

            shape
                variable global MPI dimensions.

            start
                variable offset for current MPI rank.

            count
                variable dimension for current MPI rank.

            operations
                operations to be used in this variable
        """
        variable = self._io.inquire_variable(name)

        if not variable:
            # Sequence variables
            if isinstance(content, (list, np.ndarray)):
                if isinstance(content, list):
                    content = np.asarray(content)

                # If shape, start, and count is not specified, use the numpy array's shape
                shape, start, count = self._get_variable_selection(content, shape, start, count)
                variable = self._io.define_variable(name, content, shape, start, count)
            elif ADIOS2_HAS_CUPY and isinstance(content, cp.ndarray):
                shape, start, count = self._get_variable_selection(content, shape, start, count)
                variable = self._io.define_variable(
                    name, np.array([0], dtype=content.dtype), shape, start, count
                )
            elif ADIOS2_HAS_TORCH and isinstance(content, torch.Tensor):
                shape, start, count = self._get_variable_selection(content, shape, start, count)
                var_type = str(content.dtype).rsplit(".", maxsplit=1)[-1]
                variable = self._io.define_variable(
                    name, np.array([0], dtype=var_type), shape, start, count
                )
            # Scalar variables
            elif isinstance(content, str):
                variable = self._io.define_variable(name, content)
            elif not hasattr(content, "__len__"):
                variable = self._io.define_variable(name, content, [], [], [])
            else:
                raise ValueError

        if shape != [] and not variable.single_value():
            variable.set_shape(shape)

        if start != [] or count != []:
            variable.set_selection([start, count])

        if operations:
            variable.remove_operations()
            for operation in operations:
                variable.add_operation_string(operation[0], operation[1])

        self.write(variable, content)

    def _read_var(self, variable: Variable, defer_read: bool = False):
        """
        Internal function to read when there is no preallocated buffer submitted.
        Settings must be done to Variable before the call.

        Parameters
            variable
                adios2.Variable object to be read
                Use variable.set_selection(), set_block_selection(), set_step_selection()
                to prepare a read
            defer_read
                False: read now and blocking wait for completion (Sync mode)
                True: defer reading all requests until read_complete().
                        The returned numpy array will be filled with data
                        only after calling read_complete().
        Returns
            array
                resulting array from selection
        """
        dtype = type_adios_to_numpy(variable.type())
        count = variable.count()

        if count != []:
            # array
            # steps = variable.get_steps_from_step_selection()
            # if steps == 0:
            #     steps = 1
            # Missing from C++ API: get the steps set for a variable.
            # Calculate the steps from size of count and total size that
            # we can get from the C++ API.
            size_per_step = np.prod(count)
            size_all_steps = variable.selection_size()
            steps = int(size_all_steps / size_per_step)
            if size_all_steps % size_per_step != 0:
                print(
                    f"Stream read(), step calculation for array went horribly wrong "
                    f" variable name = {variable.name()}"
                    f" selection size = {size_all_steps}  size per step = {size_per_step}"
                )

            output_shape = np.array(count)
            output_shape[0] *= steps
        else:
            # scalar
            size_all_steps = variable.selection_size()
            # if size_all_steps > 1:
            if self._mode == bindings.Mode.ReadRandomAccess and variable.steps() > 1:
                output_shape = [size_all_steps]
            else:
                output_shape = []

        mode = bindings.Mode.Sync
        if defer_read:
            mode = bindings.Mode.Deferred

        output = np.zeros(output_shape, dtype=dtype)
        self._engine.get(variable, output, mode)
        return output

    def _set_variable_settings(self, variable, start, count, block_id, step_selection):
        """
        Internal function to set the settings to Variable before getting data.

        Parameters
            variable
                adios2.Variable object to be read

            start
                variable offset dimensions

            count
                variable local dimensions from offset

            block_id
                (int) Required for reading local variables, local array, and local
                value.

            step_selection
                (list): On the form of [start, count].
        Returns
            variable
                the variable with the selection set
        """
        if step_selection is not None and not self._mode == bindings.Mode.ReadRandomAccess:
            raise RuntimeError("step_selection parameter requires 'rra' mode")

        if step_selection is not None:
            variable.set_step_selection(step_selection)

        if block_id is not None:
            variable.set_block_selection(block_id)

        if variable.type() == "string" and variable.single_value() is True:
            return variable

        if start != [] and count != []:
            variable.set_selection([start, count])
        return variable

    @singledispatchmethod
    def read_in_buffer(
        self,
        variable: Variable,
        buffer,
        start=[],
        count=[],
        block_id=None,
        step_selection=None,
        defer_read: bool = False,
    ):
        """
        Read a variable into a preallocated buffer.
        Random access read allowed to select steps.

        Parameters
            variable
                adios2.Variable object to be read
                Use variable.set_selection(), set_block_selection(), set_step_selection()
                to prepare a read

            buffer
                the pre-allocated buffer that will hold the read data
                (numpy, cupy, torch.Tensor arrays)

            start
                variable offset dimensions

            count
                variable local dimensions from offset

            block_id
                (int) Required for reading local variables, local array, and local
                value.

            step_selection
                (list): On the form of [start, count].

            defer_read
                False: read now and blocking wait for completion (Sync mode)
                True: defer reading all requests until read_complete().
                      The returned numpy array will be filled with data
                      only after calling read_complete().
        """
        variable = self._set_variable_settings(variable, start, count, block_id, step_selection)
        # make sure the buffer is a mutable array
        islist = False
        if isinstance(buffer, (list, np.ndarray)):
            islist = True
        elif ADIOS2_HAS_CUPY and isinstance(buffer, cp.ndarray):
            islist = True
        elif ADIOS2_HAS_TORCH and isinstance(buffer, torch.Tensor):
            islist = True
        if not islist:
            raise RuntimeError(
                "Cannot read single value in predefined buffers. "
                "Please use val = stream.read(var, ...) form instead"
            )

        if ADIOS2_HAS_TORCH and isinstance(buffer, torch.Tensor):
            buf_type = str(buffer.dtype).rsplit(".", maxsplit=1)[-1]
        else:
            buf_type = buffer.dtype

        dtype = type_adios_to_numpy(variable.type())
        if dtype(10).dtype != buf_type:
            raise RuntimeError(f"Read buffer type {buf_type} does not match variable type {dtype}")

        count = np.prod(variable.count())
        buf_size = buffer.size
        if ADIOS2_HAS_TORCH and isinstance(buffer, torch.Tensor):
            buf_size = buffer.numel()
        if count != buf_size:
            raise RuntimeError("Read buffer size {buf_size} does not match variable size {count}")

        mode = bindings.Mode.Sync
        if defer_read:
            mode = bindings.Mode.Deferred

        self._engine.get(variable, buffer, mode)

    @read_in_buffer.register(str)
    def _(
        self,
        name: str,
        buffer,
        start=[],
        count=[],
        block_id=None,
        step_selection=None,
        defer_read: bool = False,
    ):
        """
        Read a variable into a preallocated buffer.
        Random access read allowed to select steps.

        Parameters
            name
                variable to be read

            buffer
                the pre-allocated buffer that will hold the read data
                (numpy, cupy, torch.Tensor arrays)

            start
                variable offset dimensions

            count
                variable local dimensions from offset

            block_id
                (int) Required for reading local variables, local array, and local
                value.

            step_selection
                (list): On the form of [start, count].

            defer_read
                False: read now and blocking wait for completion (Sync mode)
                True: defer reading all requests until read_complete().
                      The returned numpy array will be filled with data
                      only after calling read_complete().
        """
        variable = self._io.inquire_variable(name)
        if not variable:
            raise ValueError()

        self.read_in_buffer(
            variable, buffer, start, count, block_id, step_selection, defer_read=defer_read
        )

    @singledispatchmethod
    def read(
        self,
        variable: Variable,
        start=[],
        count=[],
        block_id=None,
        step_selection=None,
        defer_read: bool = False,
    ):
        """
        Read a variable.
        Random access read allowed to select steps.

        Parameters
            variable
                adios2.Variable object to be read
                Use variable.set_selection(), set_block_selection(), set_step_selection()
                to prepare a read

            start
                variable offset dimensions

            count
                variable local dimensions from offset

            block_id
                (int) Required for reading local variables, local array, and local
                value.

            step_selection
                (list): On the form of [start, count].

            defer_read
                False: read now and blocking wait for completion (Sync mode)
                True: defer reading all requests until read_complete().
                        The returned numpy array will be filled with data
                        only after calling read_complete().
        Returns
            array
                resulting array from selection
        """

        variable = self._set_variable_settings(variable, start, count, block_id, step_selection)
        if variable.type() == "string" and variable.single_value() is True:
            return self._engine.get(variable)
        return self._read_var(variable, defer_read=defer_read)

    @read.register(str)
    def _(
        self,
        name: str,
        start=[],
        count=[],
        block_id=None,
        step_selection=None,
        defer_read: bool = False,
    ):
        """
        Read a variable.
        Random access read allowed to select steps.

        Parameters
            name
                variable to be read

            start
                variable offset dimensions

            count
                variable local dimensions from offset

            block_id
                (int) Required for reading local variables, local array, and local
                value.

            step_selection
                (list): On the form of [start, count].

            defer_read
                False: read now and blocking wait for completion (Sync mode)
                True: defer reading all requests until read_complete().
                        The returned numpy array will be filled with data
                        only after calling read_complete().
        Returns
            array
                resulting array from selection
        """
        variable = self._io.inquire_variable(name)
        if not variable:
            raise ValueError()

        return self.read(variable, start, count, block_id, step_selection, defer_read=defer_read)

    def read_complete(self):
        """
        Complete reading all deferred read requests.
        The returned numpy arrays of each read(..., defer_read=True) will be
        filled with data after this call.
        """
        self._engine.perform_gets()

    def write_attribute(self, name, content, variable_name="", separator="/"):
        """
        writes a self-describing single value array (numpy) variable

        Parameters
            name
                attribute name

            array
                attribute numpy array data

            variable_name
                if attribute is associated with a variable

            separator
                concatenation string between variable_name and attribute
                e.g. variable_name + separator + name ("var/attr")
                Not used if variable_name is empty
        """
        attribute = self._io.inquire_attribute(name, variable_name, separator)
        if not attribute:
            attribute = self._io.define_attribute(name, content, variable_name, separator)

    def read_attribute(self, name, variable_name="", separator="/"):
        """
        Reads a numpy based attribute

        Parameters
            name
                attribute name

            variable_name
                if attribute is associated with a variable

            separator
                concatenation string between variable_name and attribute
                e.g. variable_name + separator + name (var/attr)
                Not used if variable_name is empty

        Returns
            array
                resulting array attribute data
        """
        attribute = self._io.inquire_attribute(name, variable_name, separator)
        if not attribute:
            raise KeyError()

        if attribute.type() == "string":
            return attribute.data_string()

        return attribute.data()

    def read_attribute_string(self, name, variable_name="", separator="/"):
        """
        Reads a numpy based attribute

        Parameters
            name
                attribute name

            variable_name
                if attribute is associated with a variable

            separator
                concatenation string between variable_name and attribute
                e.g. variable_name + separator + name (var/attr)
                Not used if variable_name is empty

        Returns
            array
                resulting array attribute data
        """
        attribute = self._io.inquire_attribute(name, variable_name, separator)
        if not attribute:
            raise KeyError()

        return attribute.data_string()

    def begin_step(self, *, timeout=DEFAULT_TIMEOUT_SEC):
        """
        Write mode: declare the starting of an output step. Pass data in
        stream.write() and stream.write_attribute(). All data will be published
        in end_step().

        Read mode: in streaming mode releases the current step (no effect
        in file based engines)
        """
        if self._read_mode:
            mode = bindings.StepMode.Read
        else:
            mode = bindings.StepMode.Append

        if not self._engine.between_step_pairs():
            return self._engine.begin_step(mode=mode, timeoutSeconds=timeout)
        return bindings.StepStatus.OtherError

    def end_step(self):
        """
        Write mode: declaring the end of an output step. All data passed in
        stream.write() and all attributes passed in stream.write_attribute()
        will be published for consumers.

        Read mode: in streaming mode releases the current step (no effect
        in file based engines)
        """
        self._engine.end_step()

    def close(self):
        """
        Closes stream, thus becoming unreachable.
        Not required if using open in a with-as statement.
        Required in all other cases per-open to avoid resource leaks.
        """
        self._engine.close()
        self._engine = None
        if not self._read_mode:
            self._io.flush_all()
            self._adios.flush_all()
        self._io = None
        self._adios = None

    def current_step(self):
        """
        Inspect current step of the stream. The steps run from 0.

        Note that in a real stream, steps from the producer may be missed if
        the consumer is slow and the producer is told to discard steps
        when no one is reading them in time. You may see non-consecutive
        numbers from this function call in this case.

        Use loop_index() to get a loop counter in a for ... .steps() loop.

        Returns
            current step
        """
        return self._engine.current_step()

    def loop_index(self):
        """
        Inspect the loop counter when using for-in loops. This function returns
        consecutive numbers from 0.

        Returns
            the loop counter
        """
        return self.index

    def steps(self, num_steps=0, *, timeout=DEFAULT_TIMEOUT_SEC):
        """
        Returns an interator that can be use to itererate throught the steps.
        In each iteration begin_step() and end_step() will be internally called.

        Write Mode: num_steps is a mandatory argument and should specify the number
        of steps.

        Read Mode: if num_steps is not specified there will be as much iterations
        as provided by the actual engine. If num_steps is given and there is not
        that many steps in a file/stream, an error will occur.

        IMPORTANT NOTE: Do not use with ReadRandomAccess mode.
        """
        if not self._read_mode and num_steps == 0:
            raise RuntimeError("Stream.steps() in write mode requires num_steps")

        if self._mode == bindings.Mode.ReadRandomAccess:
            raise RuntimeError("Stream.steps() is not allowed in ReadRandomAccess mode")

        if self._mode == bindings.Mode.Read and not self.index < 0:
            raise RuntimeError(
                "Stream.steps() can only be called once in Read mode."
                " Close the stream and reopen to run another iteration loop."
            )

        if num_steps > 0:
            self.max_steps = num_steps
        else:
            self.max_steps = maxsize  # engine steps will limit the loop

        self._step_timeout_sec = timeout

        # in write mode we can run yet another loop
        self.index = -1

        return self

    def num_steps(self):
        """
        READ MODE ONLY. Return the number of steps available.
        Note that this is the steps of a file/stream. Each variable has
        its own steps, which needs to inspected with var=stream.inquire_variable() and then
        with var.steps()
        """
        if not self._read_mode:
            raise RuntimeError("num_steps requires Read/ReadRandomAccess mode")

        return self._engine.steps()

    def all_blocks_info(self, name):
        """
        Extracts all available blocks information for a particular variable. This can be
        an expensive function, memory scales up with metadata: steps and blocks per step

        Args:
            name (str): variable name

        Returns:
            list of dictionaries with information of each step
        """
        if not self._read_mode:
            raise RuntimeError("all_blocks_info requires Read/ReadRandomAccess mode")

        if self._mode == bindings.Mode.Read:
            self.begin_step()

        return self._engine.all_blocks_info(name)
