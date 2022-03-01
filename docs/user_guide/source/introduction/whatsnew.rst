##################
What's new in 2.8?
##################

Important changes to the API

  * **adios2::Mode::ReadRandomAccess** mode is introduced for reading files with access to all steps. 
    BeginStep/EndStep calls are *NOT allowed*. SetStepSelection() can be used to access specific steps in the file. 
  * **adios2::Mode::Read** mode now requires using BeginStep/EndStep loop to access steps serially one by one. Variable inquiry 
    fails outside BeginStep/EndStep sections. You need to modify your Open() statement to use the random-access mode if your
    code is accessing all steps in an existing file.
  * **adios2::ADIOS::EnterComputationBlock()**, **adios2::ADIOS::ExitComputationBlock()** are hints to ADIOS that a process is in a computing (i.e. non-communicating) phase. BP5 asynchronous I/O operations can schedule writing during such phases to avoid interfering with the application's own communication. 

New features

   * BP5 data format and engine. This new engine optimizes for many variables and many steps at large scale. 
     It is also more memory efficient than previous engines. 
   * Plugin architecture to support external *engines* and *operators* outside the ADIOS2 installation. 

Other changes

   * SST scales better for large N-to-1 staging, by managing the limits of outstanding remote direct memory access requests. 
     Of course one still introduces a literal bottleneck with such a pattern into an in situ workflow. 

