###########
ADIOS 2 FAQ
###########


MPI vs Non-MPI
**************

#. Q: Can I use the same library for MPI and non-MPI code?
   A: Short answer: No. Long answer: This created conflicts in the past, as the MPI APIs were mocked in the sequential version. 


APIs
****

#. Q: I'm a C++98 (2003) user how can I use ADIOS 2 C++11 library?
   A: Use the :ref:`C bindings`
   
#. Q: Why aren't language bindings the same, meaning C++11 APIs don't map 1-to-1 to the C APIs?
   A: Because language instrinsics are NOT THE SAME. For example, C++ and Python support key/value pair structures natively, *e.g.* std::map and dictionaries, respectively. Fortran and C only support arrays natively. Use the right language (tool) for the right task.



Titan
*****

#. Q: My application uses PGI on Titan, compilation doesn't work, what can I do?
   A: Follow directions at :ref:`Building on High Performance Computing, HPC, Systems` to setup support for PGI on Titan. Namely, do this:

   .. code-block::
    
   $ module load gcc/7.2.0
   $ makelocalrc $(dirname $(which pgc++)) -gcc $(which gcc) -gpp $(which g++) -g77 $(which gfortran) -o -net 1>${HOME}/.mypgirc 2>/dev/null
   $ module unload gcc/7.2.0
   
#. Q: I can't enable the Python bindings on Titan, what can I do?
   A: Titan default configuration is to build the static library. Python requires enabling the dynamic libraries
   
