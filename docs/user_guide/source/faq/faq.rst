##########################
Frequently Asked Questions 
##########################


MPI vs Non-MPI
**************

#. :ref:`Can I use the same library for MPI and non-MPI code?`

APIs
****

#. :ref:`Can I use ADIOS 2 C++11 library with C++98 codes?`
#. :ref:`Why are C and Fortran APIs missing functionality?`

Building on Titan
*****************

#. :ref:`My application uses PGI on Titan, can I link ADIOS 2?`
#. :ref:`How do I enable the Python bindings on Titan?`
   



FAQs Answered
*************

Can I use the same library for MPI and non-MPI code?
----------------------------------------------------

Short answer: No. Long answer: This created conflicts in the past, as the MPI APIs were mocked in the sequential version. If you need "sequencial" behavior with the MPI library, use MPI_COMM_SELF.
Always pass a communicator in the MPI version


Can I use ADIOS 2 C++11 library with C++98 codes?
-------------------------------------------------
   
Use the :ref:`C bindings`. C++11 is a brand new language standard and many new (and old, *e.g.* std::string) might cause ABI conflicts.
   
Why are C and Fortran APIs missing functionality?
-------------------------------------------------

Because language instrinsics are NOT THE SAME. For example, C++ and Python support key/value pair structures natively, *e.g.* std::map and dictionaries, respectively. Fortran and C only support arrays natively. Use the right language (tool) for the right task.


My application uses PGI on Titan, can I link ADIOS 2?
-----------------------------------------------------

Follow directions at :ref:`Building on High Performance Computing, HPC, Systems` to setup support for PGI on Titan. PGI compilers depend on GNU headers, but they must point to a version greater than gcc 4.8.1 to support C++11 features. The gcc module doesn't need to be loaded, though. Example:

   .. code-block:: bash

      $ module load gcc/7.2.0
      $ makelocalrc $(dirname $(which pgc++)) -gcc $(which gcc) -gpp $(which g++) -g77 $(which gfortran) -o -net 1>${HOME}/.mypgirc 2>/dev/null
      $ module unload gcc/7.2.0
   
   
How do I enable the Python bindings on Titan?
---------------------------------------------

ADIOS 2 default configuration on Titan is to build the static library. Python bindings require enabling the dynamic libraries and the Cray dynamic environment variable. See :ref:`Building on High Performance Computing, HPC, Systems` and  :ref:`Enabling the Python bindings`. For example:

   .. code-block:: bash

      $ CRAYPE_LINK_TYPE=dynamic cmake -DBUILD_SHARED_LIBS=ON ..
