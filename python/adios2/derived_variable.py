# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""ADIOS2 DerivedVariable Python wrapper."""


class DerivedVariable:
    """High level representation of the DerivedVariable class in the adios2.bindings"""

    def __init__(self, implementation):
        self.impl = implementation

    @property
    def impl(self):
        """Bindings implementation of the class"""
        return self._impl

    @impl.setter
    def impl(self, implementation):
        self._impl = implementation

    def __eq__(self, other):
        if isinstance(other, DerivedVariable):
            return self.name() == other.name()
        return False

    def type(self):
        """
        Type of the DerivedVariable

        Returns:
            str: Type of the DerivedVariable.
        """
        return self.impl.Type()

    def name(self):
        """
        Name of the DerivedVariable

        Returns:
            str: Name of the DerivedVariable.
        """
        return self.impl.Name()

    def add_operation_string(self, name, params={}):
        """
        Add an operation (operator) as a string.
        Only DerivedVarType.StoreData derived variables can have operations.

        Args:
            name (str): name of the operation.
            params (dict): parameters as a form of a dict for the operation.
        """
        return self.impl.AddOperation(name, params)

    def add_operation(self, operation, params={}):
        """
        Add an operation (operator).
        Only DerivedVarType.StoreData derived variables can have operations.

        Args:
            operation (Operator): the operation.
            params (dict): parameters as a form of a dict for the operation.
        """
        return self.impl.AddOperation(operation.impl, params)

    def operations(self):
        """
        Current operations (operators) assigned to this DerivedVariable.

        Returns:
            list(Operators): operators assigned.
        """
        return self.impl.Operations()

    def remove_operations(self):
        """
        Remove operations (operators) assigned to this DerivedVariable.
        """
        self.impl.RemoveOperations()
