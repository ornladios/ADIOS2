####################
Tutorial: ADIOS2 101
####################

********
Setup
********

- Install adios2
- Go to the tutorial dir
- cmake
- cmake --build


******************
(hello)adios world
******************

Solution at `tutorials/00_helloworld/helloworld_solution.cxx`
Source at `tutorials/00_helloworld/helloworld.cxx`

Open source.

In the writer side:

1. First we need to initialize ADIOS2

...

2. Second we need to declare an IO object which means ...

...

3. Then we can create an engine, which we have many types of ...

...

4. Now create a simpel variable ...

Pass an string ...

In the reader side:

5. repeat 1-4 steps.

...

6. Now read the variable

...

**********************************
(hello)adios world (Low level API)
**********************************

Solution at `tutorials/00_helloworld_basics/helloworld_solution.cxx`
Source at `tutorials/00_helloworld_basics/helloworld.cxx`

Open source.

In the writer side:

1. First we need to initialize ADIOS2

...

2. Second we need to declare an IO object which means ...

...

3. Then we can create an engine, which we have many types of ...

...

4. Now create a simpel variable ...

Pass an string ...

In the reader side:

5. repeat 1-4 steps.

...

6. Now read the variable

...


*********
Variables
*********

Concept of steps
Concept of Variables shape

Solution at `tutorials/01_variables/variables_solution.cxx`
Source at `tutorials/01_variables/variables.cxx`

*********
Operators
*********

Concept of Operators

Solution at `tutorials/02_operators/operators_solution.cxx`
Source at `tutorials/02_operators/operators.cxx`
