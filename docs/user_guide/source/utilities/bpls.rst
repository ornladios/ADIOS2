****
bpls
****

The `bpls` utility is for examining and pretty-printing the content of ADIOS output files.
By default, it lists the variables in the file including
the type, name, and dimensionality. 

Let's assume we run the Heat Transfer tutorial example and produce the output by

.. code-block:: bash

    $ mpirun -n 12 ./heatSimulation  sim.bp  3 4  4 5 3 1
    Process decomposition  : 3 x 4
    Array size per process : 4 x 5
    Number of output steps : 3
    Iterations per step    : 1

    $ mpirun -n 3 ./heatAnalysis sim.bp a.bp 3 1

    $ bpls a.bp
      double   T     3*{12, 20}
      double   dT    3*{12, 20}

In our example, we have two arrays, `T` and `dT`. Both are 2-dimensional `double` arrays, their global size is `12x20` and the file contains `3 output steps` of these arrays.

.. note::

    bpls is written in C++ and therefore sees the order of the dimensions in `row major`. If the data was written from Fortran in column-major order, you will see the dimension order flipped when listing with bpls, just as a code written in C++ or python would see the data. 

 
Here is the description of additional options
(use `bpls -h` to print help on all options for this utility).


* **-l**

  Print the min/max of the arrays and the values of scalar values
  
  .. code-block:: bash

    $ bpls -l a.bp
      double   T     3*{12, 20} = 2.17453e-06 / 200
      double   dT    3*{12, 20} = -41.384 / 40.2627

* **-a**

  List the attributes along with the variables 

  .. code-block:: bash

    $ bpls a.bp -la
      double   T               3*{12, 20} = 2.17453e-06 / 200
      string   T/description   attr   = "Temperature from heatSimulation"
      string   T/unit          attr   = "C"
      double   dT              3*{12, 20} = -41.384 / 40.2627
      string   dT/description  attr   = "Temperature difference between two steps calculated in heatAnalysis"




