######################
Command Line Utilities
######################

ADIOS 2 provides a set of command line utilities for quick data exploration and 
manipulation that builds on top of the library. The are located in the inside the ``adios2-install-location/bin`` directory after a ``make install`` 

.. tip::

   Optionally the ``adios2-install-location/bin`` location can be added to your PATH to avoid absolute paths when using adios2 command line utilities   


Currently supported tools are:

* *bpls* : exploration of bp/hdf5 files data and metadata in human readable formats   
* *adios_reorganize*
* *adios2-config*

.. include:: bpls.rst
.. include:: adios_reorganize.rst
.. include:: adios2-config.rst
