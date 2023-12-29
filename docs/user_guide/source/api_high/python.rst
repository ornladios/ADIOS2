*********************
Python High-Level API
*********************

Python simple bindings follow closely Python style directives. Just like the full APIs, they rely on numpy and, optionally, on ``mpi4py``, if the underlying ADIOS2 library is compiled with MPI.

For online examples on MyBinder :

- `Python-MPI Notebooks <https://mybinder.org/v2/gh/ornladios/ADIOS2-Jupyter.git/python-mpi>`_

- `Python-noMPI Notebooks <https://mybinder.org/v2/gh/ornladios/ADIOS2-Jupyter.git/python-nompi>`_


Python Write example
--------------------

.. code-block:: python
   
   from mpi4py import MPI
   import numpy as np
   from adios2 import Stream
   
   comm = MPI.COMM_WORLD
   rank = comm.Get_rank()
   size = comm.Get_size()
   ...   
   shape = [size * nx]
   start = [rank * nx]
   count = [nx]
   
   with Stream("cfd.bp", "w", comm) as s:
      # NSteps from application
      for _ in s.steps(NSteps):
         if rank == 0 and s.current_step() == 0:
            fh.write("size", np.array([size]))
         
         fh.write("physical_time", np.array([physical_time]) )
         # temperature and pressure are numpy arrays
         fh.write("temperature", temperature, shape, start, count)
         # advances to next step
         fh.write("pressure", pressure, shape, start, count, end_step=True)


Python Read "step-by-step" example
----------------------------------

.. code-block:: python
   
   from mpi4py import MPI
   import numpy as np
   from adios2 import Stream
   
   comm = MPI.COMM_WORLD
   rank = comm.Get_rank()
   size = comm.Get_size()
   ...   
   shape = [size * nx]
   start = [rank * nx]
   count = [nx]
   
   if rank == 0:
       # if only one rank is active pass MPI.COMM_SELF
       with Stream("cfd.bp", "r", MPI.COMM_SELF) as s:
           for _ in s.steps():
               # inspect variables in current step
               for name, info in s.available_variables():
                   print("variable_name: " + name)
                   for key, value in info.items():
                      print("\t" + key + ": " + value)
                   print("\n")
            
               # track current step
               if s.current_step() == 0:
                  size_in = s.read("size") 
               
               # read variables return a numpy array with corresponding selection
               physical_time = s.read("physical_time")
               temperature = s.read("temperature", start, count)
               pressure = s.read("pressure", start, count)

.. caution::
   
   When reading in stepping mode with the for-in directive, as in the example above, you can ignore the variable that iterates the s.step() as it contains the same ref of the Stream instance. 


High Level API
--------------

.. autoclass:: adios2::Stream
    :members:

.. autoclass:: adios2::FileReader
    :members:

.. autoclass:: adios2::Adios
    :members:

.. autoclass:: adios2::IO
    :members:

.. autoclass:: adios2::Engine
    :members:

.. autoclass:: adios2::Variable
    :members:

.. autoclass:: adios2::Attribute
    :members:

.. autoclass:: adios2::Operator
    :members:
