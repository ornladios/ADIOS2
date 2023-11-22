"""License:
  Distributed under the OSI-approved Apache License, Version 2.0.  See
  accompanying file Copyright.txt for details.
"""


class Attribute:
    """High level representation of the Attribute class in the adios2.bindings"""

    def __init__(self, io, name, *args, **kwargs):
        self.impl = io.DefineAttribute(name, *args, **kwargs)

    @property
    def impl(self):
        """Bindings implementation of the class"""
        return self._impl

    @impl.setter
    def impl(self, implementation):
        self._impl = implementation

    def __eq__(self, other):
        if isinstance(other, Attribute):
            return self.name() == other.name()
        return False

    def name(self):
        """
        Name of the Attribute

        Returns:
            Name of the Attribute as a str.
        """
        return self.impl.Name()

    def type(self):
        """
        Type of the Attribute

        Returns:
            Type of the Attribute as a str.
        """
        return self.impl.Type()

    def data(self):
        """
        Content of the Attribute

        Returns:
            Content of the Attribute as a non string.
        """
        return self.impl.Data()

    def data_string(self):
        """
        Content of the Attribute

        Returns:
            Content of the Attribute as a str.
        """
        return self.impl.DataString()
