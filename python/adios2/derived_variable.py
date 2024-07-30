"""License:
  Distributed under the OSI-approved Apache License, Version 2.0.  See
  accompanying file Copyright.txt for details.
"""


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
