*************
Running Tests
*************

ADIOS2 uses `googletest <https://github.com/google/googletest>`_ to enable automatic testing after a CMake build. To run tests just type after building with make, run:

.. code-block:: bash

    $ ctest    
      or  
    $ make test

The following screen will appear providing information on the status of each finalized test:

.. code-block:: bash

    Test project /home/wfg/workspace/build
    Start  1: ADIOSInterfaceWriteTest.DefineVarChar1x10
    1/46 Test  #1: ADIOSInterfaceWriteTest.DefineVarChar1x10 .......................   Passed    0.06 sec
    Start  2: ADIOSInterfaceWriteTest.DefineVarShort1x10
    2/46 Test  #2: ADIOSInterfaceWriteTest.DefineVarShort1x10 ......................   Passed    0.04 sec
    Start  3: ADIOSInterfaceWriteTest.DefineVarInt1x10
    3/46 Test  #3: ADIOSInterfaceWriteTest.DefineVarInt1x10 ........................   Passed    0.04 sec
    Start  4: ADIOSInterfaceWriteTest.DefineVarLong1x10
    ... 
    44/46 Test #44: ADIOSZfpWrapper.Float100 ........................................   Passed    0.00 sec
    Start 45: ADIOSZfpWrapper.UnsupportedCall
    45/46 Test #45: ADIOSZfpWrapper.UnsupportedCall .................................   Passed    0.00 sec
    Start 46: ADIOSZfpWrapper.MissingMandatoryParameter
    46/46 Test #46: ADIOSZfpWrapper.MissingMandatoryParameter .......................   Passed    0.00 sec

    100% tests passed, 0 tests failed out of 46

    Total Test time (real) =   2.04 sec


