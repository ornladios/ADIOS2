##########
As Package
##########

*****
Conda
*****

The ADIOS 2 Python bindings can be obtained from anaconda cloud:

* x86-64 and MacOS: `williamfgc adios2-openmpi adios2-mpich adios2-nompi <https://anaconda.org/williamfgc>`_
* Multiple archs: `conda-forge adios2 <https://anaconda.org/conda-forge/adios2>`_

A recently tested install sequence is

.. code-block:: bash

    $ conda upgrade --all
    $ conda install -c conda-forge adios2


On Mac, you may need to run

.. code-block:: bash

    $ brew install zeromq

and do a source build of `blosc <https://github.com/Blosc/c-blosc.git>`_.


*****
Spack
*****

ADIOS 2 is available via the Spack package `adios2 <https://spack.readthedocs.io/en/latest/package_list.html#adios2>`_.


******
Docker
******

A Dockerfile demonstrating building and installing ADIOS 2 is available `here <https://github.com/ornladios/ADIOS2/tree/master/scripts/docker/images/ubuntu18.04/Dockerfile>`_.
