*********************
Python high-level API
*********************

Python simple bindings follow closely the :ref:`C++ simple bindings`. Just like the full APIs, they rely on numpy and (optionally) on ``mpi4py`` if ADIOS2 is compiled with MPI.

For online examples on MyBinder :

- `Python-MPI Notebooks <https://mybinder.org/v2/gh/ornladios/ADIOS2-Jupyter.git/python-mpi>`_

- `Python-noMPI Notebooks <https://mybinder.org/v2/gh/ornladios/ADIOS2-Jupyter.git/python-nompi>`_


Python Write example
--------------------

.. code-block:: python
   
   from mpi4py import MPI
   import numpy as np
   import adios2
   
   comm = MPI.COMM_WORLD
   rank = comm.Get_rank()
   size = comm.Get_size()
   ...   
   shape = [size * nx]
   start = [rank * nx]
   count = [nx]
   
   # with-as will call adios2.close on fh at the end
   with adios2.open("cfd.bp", "w", comm) as fh:

      # NSteps from application
      for i in range(0, NSteps):
      
         if(rank == 0 and i == 0):
            fw.write("size", np.array([size]))
         
         fw.write("physical_time", np.array([physical_time]) )
         # temperature and pressure are numpy arrays
         fw.write("temperature", temperature, shape, start, count)
         # advances to next step
         fw.write("pressure", pressure, shape, start, count, end_step=True)


Python Read "step-by-step" example
----------------------------------

.. code-block:: python
   
   from mpi4py import MPI
   import numpy as np
   import adios2
   
   comm = MPI.COMM_WORLD
   rank = comm.Get_rank()
   size = comm.Get_size()
   ...   
   shape = [size * nx]
   start = [rank * nx]
   count = [nx]
   
   if( rank == 0 ):
      # with-as will call adios2.close on fh at the end
      # if only one rank is active pass MPI.COMM_SELF
      with adios2.open("cfd.bp", "r", MPI.COMM_SELF) as fh:
      
         for fstep in fh:

            # inspect variables in current step
            step_vars = fstep.available_variables()
            
            # print variables information
            for name, info in step_vars.items():
                print("variable_name: " + name)
                for key, value in info.items():
                   print("\t" + key + ": " + value)
                print("\n")
            
            # track current step
            step = fh_step.currentstep()
            if( step == 0 ):
               size_in = fh_step.read("size") 
            
            # read variables return a numpy array with corresponding selection
            physical_time = fh_step.read("physical_time")
            temperature = fh_step.read("temperature", start, count)
            pressure = fh_step.read("pressure", start, count)

.. caution::
   
   When reading in stepping mode with the for-in directive, as in the example above, use the step handler (``fstep``) inside the loop rather than the global handler (``fh``) 


File class API
--------------

.. automodule:: adios2
    :members: open

.. autoclass:: adios2::File
    :members:
