****************************
Enabling the Python bindings
****************************

To enable the Python bindings in ADIOS2, based on `PyBind11 <http://pybind11.readthedocs.io/en/stable/>`_, make sure to follow these guidelines:

1. **Dynamic library support:** The Python bindings in ADIOS2 require that your platform supports shared libraries, which for Cray systems (and some BlueGene systems) can require additional steps. See the previous section on :ref:`Building on High Performance Computing, HPC, Systems`. Note that you can still build the Python bindings with a static libadios2.a library, as long as your platform supports it.

2. **Minimum requirements:** 

    * Python 2.7 and above.
    * Corresponding NumPy for the Python version used for the build  
    * If MPI is enabled, a compatible version of mpi4py needs to be installed.
    * If Python versions 2.7 and 3 are installed in the system, CMake will autodetect Python 3 requirements as the default option. A Python executable can be passed explicitly to CMake builds using `-DPYTHON_EXECUTABLE=...` 

3. **Running:** if CMake enables Python compilation, an adios2.so library containing the Python modules is generated in the build directory under lib/pythonX.X/site-packages/ 

    * make sure your `PYTHONPATH` environment variable contains the path to the adios2.so module.
    
    * make sure the Python interpreter is compatible with the version used for compilation. 
    
    * try running Python tests with: `ctest -R Python`
    
    * try running the `helloBPWriter.py <https://github.com/ornladios/ADIOS2/blob/master/examples/hello/bpWriter/helloBPWriter.py>`_ example under ADIOS2/examples/hello/bpWriter, and `helloBPTimeWriter.py <https://github.com/ornladios/ADIOS2/blob/master/examples/hello/bpTimeWriter/helloBPTimeWriter.py>`_ under ADIOS2/examples/hello/bpTimeWriter

    .. code-block:: bash
       
        Python 2.7:
            $ mpirun -n 4 python helloBPWriter.py  
            $ python helloBPWriter.py
       
        Python 3:
            $ mpirun -n 4 python3 helloBPWriter.py  
            $ python3 helloBPWriter.py
   