.. _HPCBuild:

***********************
Building on HPC Systems
***********************

#. **Modules:** Make sure all "module" dependencies are loaded and that minimum requirements are satisfied.
   Load the latest CMake module as many HPC systems default to an outdated version.
   Build with a C++11-compliant compiler, such as gcc >= 4.8.1, Intel >= 15, and PGI >= 15.

#. **Static/Dynamic build:** On Cray systems such as `Titan <https://www.olcf.ornl.gov/kb_articles/compiling-and-node-types/>`_,
   the default behavior is static linkage, thus CMake builds ADIOS2 creates the static library ``libadios2.a`` by default.
   Read the system documentation to enable dynamic compilation, usually by setting an environment variable such as ``CRAYPE_LINK_TYPE=dynamic``.
   Click `here <https://github.com/ornladios/ADIOS2/tree/master/scripts/runconf/runconf_olcf.sh>`_ for a fully configurable script example on OLCF systems.

#. **Big Endian and 32-bit systems:** ADIOS2 hasn't been tested on big endian or 32-bit systems. Please be aware before attempting to run.

#. **CMake minimum version:** The ADIOS2 build system requires a minimum CMake version of 3.6.0. However, IBM XL, Cray, and PGI compilers require version 3.9.0 or newer.

#. **PGI compilers and C++11 support:** Version 15 of the PGI compiler is C++11 compliant.
   However it relies on the C++ standard library headers supplied by the system version of GCC, which may or may support all the C++11 features used in ADIOS2.
   On many systems (Titan at OLCF, for example) even though the PGI compiler supports C++11, the configured GCC and its headers do not (4.3.x on Cray Linux Environment, and v5 systems like Titan).
   To configure the PGI compiler to use a newer GCC, you must create a configuration file in your home directory that overrides the PGI compiler's default configuration.
   On Titan, the following steps will re-configure the PGI compiler to use GCC 6.3.0 provided by a module:

.. code-block:: bash

  $ module load gcc/6.3.0
  $ makelocalrc $(dirname $(which pgc++)) -gcc $(which gcc) -gpp $(which g++) -g77 $(which gfortran) -o -net 1>${HOME}/.mypgirc 2>/dev/null
