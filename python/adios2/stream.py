"""License:
  Distributed under the OSI-approved Apache License, Version 2.0.  See
  accompanying file Copyright.txt for details.
"""

from adios2.adios import Adios
from adios2 import bindings

import numpy as np


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
    }[name]


class Stream:
    """High level implementation of the Stream class from the core API"""

    def __init__(self, path, mode="r", comm=None, engine_type="BPStream", config_file=None):
        if comm and not bindings.is_built_with_mpi:
            raise RuntimeError("Cannot use MPI since ADIOS2 was built without MPI support")

        # pylint: disable=E1121
        if config_file:
            if comm:
                self._adios = Adios(config_file, comm)
            else:
                self._adios = Adios(config_file)
        else:
            if comm:
                self._adios = Adios(comm)
            else:
                self._adios = Adios()
        # pylint: enable=E1121
        self._io_name = f"stream:{path}:engine_type:{engine_type}:mode:{mode}"
        self._io = self._adios.declare_io(self._io_name)

        if mode == "r":
            self._mode = bindings.Mode.Read
        elif mode == "rra":
            self._mode = bindings.Mode.ReadRandomAccess
        elif mode == "w":
            self._mode = bindings.Mode.Write
        else:
            raise ValueError()

        self._engine = self._io.open(path, self._mode)
        self.index = 0
        self.max_steps = 0

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
        if self.index > 0:
            self.end_step()
        if self.index == self.max_steps:
            self.index = 0
            self.max_steps = 0
            raise StopIteration

        self.index += 1
        self.begin_step()
        return self

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

    def set_transport(self, transport, parameters={}):
        """
        Adds a transport and its parameters to current IO. Must be
        supported by current engine type.

        Parameters
            type
                must be a supported transport type for current engine.

            parameters
                acceptable parameters for a particular transport
                CAN'T use the keywords "Transport" or "transport" in key

        Returns
            transport_index
                handler to added transport
        """
        self._io.add_transport(transport, parameters)

    def available_variables(self):
        """

        Returns a 2-level dictionary with variable information.
        Read mode only.

        Parameters
            keys
               list of variable information keys to be extracted (case insensitive)
               keys=['AvailableStepsCount','Type','Max','Min','SingleValue','Shape']
               keys=['Name'] returns only the variable names as 1st-level keys
               leave empty to return all possible keys

        Returns
            variables dictionary
                key
                    variable name
                value
                    variable information dictionary
        """
        return self._io.available_variables()

    def available_attributes(self):
        """
        Returns a 2-level dictionary with attribute information.
        Read mode only.

        Returns
            attributes dictionary
                key
                    attribute name
                value
                    attribute information dictionary
        """
        return self._io.available_attributes()

    def define_variable(self, name):
        """
        Define new variable without specifying its type and content.
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

    def write(self, name, content, shape=[], start=[], count=[], operations=None):
        """
        writes a variable

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
            # Sequences variables
            if isinstance(content, np.ndarray):
                variable = self._io.define_variable(name, content, shape, start, count)
            elif isinstance(content, list):
                content_np = np.array(content)
                variable = self._io.define_variable(name, content_np, shape, start, count)
            # Scalars variables
            elif isinstance(content, str) or not hasattr(content, "__len__"):
                variable = self.define_variable(name)
            else:
                raise ValueError

        if shape != [] and not variable.single_value():
            variable.set_shape(shape)

        if start != [] and count != []:
            variable.set_selection([start, count])

        if operations:
            variable.remove_operations()
            for operation in operations:
                variable.add_operation_string(operation[0], operation[1])

        if isinstance(content, list):
            content_np = np.array(content)
            self._engine.put(variable, content_np, bindings.Mode.Sync)
        else:
            self._engine.put(variable, content, bindings.Mode.Sync)

    def read(self, name, start=[], count=[], block_id=None, step_selection=None):
        """
        Random access read allowed to select steps,
        only valid with Stream Engines

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
        Returns
            array
                resulting array from selection
        """
        variable = self._io.inquire_variable(name)
        if not variable:
            raise ValueError()

        if step_selection and not self.mode == bindings.Mode.ReadRandomAccess:
            raise RuntimeError("step_selection parameter requires 'rra' mode")

        if step_selection:
            variable.set_step_selection(step_selection)

        if block_id:
            variable.set_block_selection(block_id)

        if variable.type() == "string" and variable.single_value() is True:
            return self._engine.get(variable)

        if start != [] and count != []:
            variable.set_selection([start, count])

        output_shape = (variable.selection_size(),)
        if count != []:
            if step_selection:
                output_shape = np.array(count) * step_selection[1]
            else:
                output_shape = count

        dtype = type_adios_to_numpy(variable.type())

        output = np.zeros(output_shape, dtype=dtype)
        self._engine.get(variable, output)
        return output

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

    def begin_step(self):
        """
        Write mode: advances to the next step. Convenient when declaring
        variable attributes as advancing to the next step is not attached
        to any variable.

        Read mode: in streaming mode releases the current step (no effect
        in file based engines)
        """

        if not self.engine.between_step_pairs():
            self.engine.begin_step()

    def end_step(self):
        """
        Write mode: advances to the next step. Convenient when declaring
        variable attributes as advancing to the next step is not attached
        to any variable.

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
        self._io.flush_all()
        self._io = None
        self._adios.flush_all()
        self._adios = None

    def current_step(self):
        """
        Inspect current step when using for-in loops, read mode only

        Returns
            current step
        """
        return self._engine.current_step()

    def steps(self, num_steps=0):
        """
        Returns an interator that can be use to itererate throught the steps.
        In each iteration begin_step() and end_step() will be internally called.

        Write Mode: num_steps is a mandatory argument and should specify the number
        of steps.

        Read Mode: num_steps should not be used and there will be as much iterations
        as steps exits.

        IMPORTANT NOTE: Do not use with ReadRandomAccess mode.
        """
        if num_steps > 0:
            self.max_steps = num_steps
        else:
            self.max_steps = self._engine.steps()

        self.index = 0
        return self

    def num_steps(self):
        """READ MODE ONLY. Return the number of steps available."""
        if self.mode not in (bindings.Mode.ReadRandomAccess, bindings.Mode.Read):
            raise RuntimeError("num_steps requires Read/ReadRandomOnly mode")

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
        if self.mode not in (bindings.Mode.ReadRandomAccess, bindings.Mode.Read):
            raise RuntimeError("all_blocks_info requires Read/ReadRandomOnly mode")

        if self.mode == bindings.Mode.Read:
            self.begin_step()

        return self.engine.all_blocks_info(name)
