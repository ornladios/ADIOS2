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

 
Here is the description of the most used options
(use `bpls -h` to print help on all options for this utility).


* **-l**

  Print the min/max of the arrays and the values of scalar values
  
  .. code-block:: bash

    $ bpls -l a.bp
      double   T     3*{12, 20} = 2.17453e-06 / 200
      double   dT    3*{12, 20} = -41.384 / 40.2627

* **-a** **-A**

  List the attributes along with the variables. `-A` will print the attributes only.

  .. code-block:: bash

    $ bpls a.bp -la
      double   T               3*{12, 20} = 2.17453e-06 / 200
      string   T/description   attr   = "Temperature from simulation"
      string   T/unit          attr   = "C"
      double   dT              3*{12, 20} = -41.384 / 40.2627
      string   dT/description  attr   = "Temperature difference between two steps calculated in analysis"

* `pattern`

  Select which variables/attributes to list or dump. By default the pattern is like a shell file pattern. Using the -e option the pattern will be used as an regular expression

  
  .. code-block:: bash

    $ bpls a.bp -la T*
      double   T               3*{12, 20} = 2.17453e-06 / 200
      
  .. code-block:: bash

    $ bpls a.bp -la T.* -e
      double   T               3*{12, 20} = 2.17453e-06 / 200
      string   T/description   attr   = "Temperature from simulation"
      string   T/unit          attr   = "C"

  .. code-block:: bash

    $ bpls a.bp -la T/* dT/* 
      string   T/description   attr   = "Temperature from simulation"
      string   T/unit          attr   = "C"
      string   dT/description  attr   = "Temperature difference between two steps calculated in analysis"

* **-D**

  Print the decomposition of a variable. In the BP file, the data blocks written by different writers are stored separately and have their own size info and min/max statistics. This option is useful at code development to check if the output file is written the way intended.


  .. code-block:: bash

    $ bpls a.bp -l T -D
      double   T               3*{12, 20} = 2.17453e-06 / 200
            step 0: 
              block 0: [ 0: 3,  0:19] = 2.17453e-06 / 169.366
              block 1: [ 4: 7,  0:19] = 36.2402 / 200
              block 2: [ 8:11,  0:19] = 2.17453e-06 / 169.366
            step 1: 
              block 0: [ 0: 3,  0:19] = 34.4583 / 129.104
              block 1: [ 4: 7,  0:19] = 53.9598 / 164.721
              block 2: [ 8:11,  0:19] = 34.4583 / 139.093
            step 2: 
              block 0: [ 0: 3,  0:19] = 47.8894 / 117.111
              block 1: [ 4: 7,  0:19] = 61.6086 / 149.78
              block 2: [ 8:11,  0:19] = 48.6223 / 128.216

  In this case we find 3 blocks per output step and 3 output steps. We can see that the variable `T` was decomposed in the first (slow) dimension. In the above example, the `T` variable in the simulation output (sim.bp) had 12 blocks per step, but the analysis code was running on 3 processes, effectively reorganizing the data into fewer larger blocks.


* **-d**

  Dump the data content of a variable. For pretty-printing, one should use the additional `-n` and `-f` options. For selecting only a subset of a variable, one should use the `-s` and `-c` options.
