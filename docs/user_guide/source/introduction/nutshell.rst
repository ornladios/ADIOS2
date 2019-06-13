********************
ADIOS2 in a Nutshell
********************

**ADIOS2** is the latest implementation of the `ADaptable Input Output System, ADIOS <https://www.olcf.ornl.gov/center-projects/adios>`_.
This brand new architecture was designed to continue supporting the performance
legacy of ADIOS, and extend its current capabilities to address current and
future input/output (IO) challenges in the scientific data lifecycle through
effective research and development (R&D) activities.   

ADIOS2 infrastructure is developed as a multi-institutional collaboration
between:  

  * `Oak Ridge National Laboratory <https://www.ornl.gov>`_  
  * `Kitware Inc. <https://www.kitware.com>`_  
  * `Lawrence Berkeley National Laboratory <http://www.lbl.gov>`_   
  * `Georgia Institute of Technology <http://www.gatech.edu>`_   
  * `Rutgers University <http://www.rutgers.edu>`_

The key aspects of the ADIOS2 infrastructure include:    
  
#. **Modular architecture:** ADIOS2 takes advantage of the major features
   added in C++11. The architecture offers a balanced combination of runtime
   Object-Oriented and static Template components to achieve the right level of
   granularity to reuse components and cover a broad range of IO applications.

  
#. **Community:** by creating a set of coding standards, GitHub collaboration
   workflows and the proper level of documentation: Doxygen API, code comments,
   git issues and commits, ADIOS2 eases the collaboration aspects for community
   engaging.

   
#. **Sustainability:** automatic nightly build, testing and continuous
   integration to ensure the quality of the contributions in our software. Please report any issue on github: https://github.com/ornladios/ADIOS2/issues   


#. **Language Support:** ADIOS2 is written in full taking advantage of the
   C++11 major standard update. In addition, it started providing bindings for
   seamless support in Python, C, Fortran and Matlab.  


#. **Commitment:** ADIOS2 is committed to the community of researchers, users
   and developers to release a new version every 6 months.

**ADIOS2 is funded by the Department of Energy, as part of the Exascale Computing Program.** 

************************
What ADIOS2 is and isn't
************************

**ADIOS2 is:**:

1. **A Unified I/O Performance Framework**: using the same abstraction API ADIOS2 would Transport and Transform groups of self-describing data variables and attributes across different media (file, wide-area-network, in-memory staging, etc.) with performance an ease of use as the main goals.

2. **MPI-based**: out-of-box MPI-based, non-MPI is optional at build time.

3. **Group-based**: adios2 favors a deferred/prefetch/grouped variables transport mode by default. Sync mode, one variable at a time, is the special case.

4. **Step-based**: to resemble actual production of data in "steps" of variable groups, for either streaming or random-access (file) media

5. **Free and open-source** 


**ADIOS2 is not**:

1. **A File-only I/O library**

2. **MPI-only**

3. **A Database**

4. **A Data Hierarchy Model**: they build on top of the adios2 library according to the application

5. **A Memory Manager Library**: we don't own or manage the application's memory

6. **Difficult**
