****************
Fortran bindings
****************

.. role:: f90(code)
   :language: fortran
   :class: highlight

The Fortran bindings API consist entirely on subroutines calls. Always, the 1st argument is a Fortran type (struct) to an ADIOS2 component, while the last argument is an error integer flag, :f90:`integer ierr`. ``ierr==0`` means normal execution while any other value represents an error or a different state. ADIOS2 Fortran bindings provide a list of possible errors coming from the C++ standardized error exception library:

.. code-block:: fortran

    ! error possible values for ierr
    integer, parameter :: adios2_error_none = 0
    integer, parameter :: adios2_error_invalid_argument = 1,
    integer, parameter :: adios2_error_system_error = 2,
    integer, parameter :: adios2_error_runtime_error = 3,
    integer, parameter :: adios2_error_exception = 4

Click here for a `Fortran write and read example`_ to illustrate the use of the APIs calls before digging into the description of each subroutine. This test will compile under your build/bin/ directory.

.. _`Fortran write and read example`: https://github.com/ornladios/ADIOS2/blob/master/testing/adios2/bindings/fortran/TestBPWriteReadHeatMap3D.f90

The following subsections describe the overall component representation and the main subroutines versions in the Fortran bindings API.

ADIOS2 typed handlers
---------------------

ADIOS2 Fortran bindings handlers are mapped 1-to-1 to the ADIOS components described in the :ref:`Components Overview` section. For convenience, each type handler contains descriptive components used for read-only inspection.
 
.. code-block:: fortran

   type(adios2_adios) :: adios
   type(adios2_io) :: io
   type(adios2_variable) :: variable
   type(adios2_attribute) :: attribute
   type(adios2_engine) :: engine
   
   !Read-only components for inspection and ( = defaults)
   
   type adios2_adios
        logical :: valid = .false.
    end type

    type adios2_io
        logical :: valid = .false.
        character(len=15):: engine_type = 'BPFile'
    end type

    type adios2_variable
        logical :: valid = .false.
        character(len=4095):: name = ''
        integer :: type = -1
        integer :: ndims = -1
    end type

    type adios2_attribute
        logical :: valid = .false.
        character(len=4095):: name = ''
        integer :: type = -1
        integer :: length = 0
    end type

    type adios2_engine
        logical :: valid = .false.
        character(len=63):: name = ''
        character(len=15):: type = ''
        integer :: mode = adios2_mode_undefined
    end type

    type adios2_operator
        logical :: valid = .false.
        character(len=63):: name = ''
        character(len=63):: type = ''
    end type
   

.. caution::

   Use the type read-only components for information purposes only.
   Changing their values directly, *e.g.* ``variable%name = new_name`` does not have any effect inside the adios2 library 
   

:ref:`ADIOS` subroutines
------------------------

* :f90:`subroutine adios2_init` starting point for the adios2 library 

   .. code-block:: fortran

      ! MPI versions
      ! Debug mode = ON (.true.) by default
      subroutine adios2_init(adios, comm, ierr)
      subroutine adios2_init(adios, config_file, comm, ierr)
      
      subroutine adios2_init(adios, comm, adios2_debug_mode, ierr)
      subroutine adios2_init(adios, config_file, comm, adios2_debug_mode, ierr)
      
      ! Non-MPI serial versions
      ! Debug mode = ON (.true.) by default
      subroutine adios2_init(adios, ierr)
      subroutine adios2_init(adios, config_file, ierr) 
      
      subroutine adios2_init(adios, adios2_debug_mode, ierr)
      subroutine adios2_init(adios, config_file, adios2_debug_mode, ierr)
   
      ! WHERE:
      
      ! adios handler to allocate
      type(adios2_adios), intent(out):: adios 
      
      ! MPI Communicator
      integer, intent(in):: comm 
      
      ! Optional runtime configuration file (*.xml), see Runtime Configuration Files
      character*(*), intent(in) :: config_file
      
      ! .true. (adios2_debug_mode_on): enable extra user input checks-> recommended
      ! .false. (adios2_debug_mode_of): disable extra user input checks
      logical, intent(in):: adios2_debug_mode
      

* :f90:`subroutine adios2_declare_io` spawn io components

   .. code-block:: fortran

      subroutine adios2_declare_io(io, adios, io_name, ierr)
      
      ! WHERE:
      
      ! io component that defines an IO tasks inside adios component
      type(adios2_io), intent(out):: io
      
      ! adios component from adios2_init spawning io tasks 
      type(adios2_adios), intent(in):: adios
      
      ! unique name associated with this io component inside adios
      character*(*), intent(in):: io_name

* :f90:`subroutine adios2_at_io` retrieve an existing io component, useful when the original handler for adios2_declare_io goes out of scope

   .. code-block:: fortran

      subroutine adios2_at_io(io, adios, io_name, ierr)
      
      ! WHERE:
      
      ! io component that defines an IO tasks inside adios component
      type(adios2_io), intent(out):: io
      
      ! adios component from adios2_init that owns io tasks 
      type(adios2_adios), intent(in):: adios
      
      ! unique name associated with an existing io component (created with adios2_declare_io)
      character*(*), intent(in):: io_name


* :f90:`subroutine adios2_flush_all` flush all current engines in all ios

   .. code-block:: fortran

      subroutine adios2_flush_all(adios, ierr)
      
      ! WHERE:
      
      ! adios component from adios2_init owning ios and engines 
      type(adios2_adios), intent(in):: adios  
   
* :f90:`subroutine adios2_finalize` final point for the adios component

   .. code-block:: fortran

      subroutine adios2_finalize(adios, ierr)
      
      ! WHERE:
      
      ! adios handler to be deallocated 
      type(adios2_adios), intent(in):: adios


.. caution::
   
   Make sure that for every call to ``adios2_init`` there is a call to ``adios2_finalize`` for the same adios handler. Not doing so will result in memory leaks. 

      
:ref:`IO` subroutines
---------------------
      
* :f90:`subroutine adios2_define_variable` 

   .. code-block:: fortran

      ! Global array variables
      subroutine adios2_define_variable(variable, io, variable_name, adios2_type, &
                                        ndims, shape_dims, start_dims, count_dims, & 
                                        adios2_constant_dims, ierr) 
      ! Global single value variables
      subroutine adios2_define_variable(variable, io, variable_name, adios2_type, ierr)
      
      ! WHERE:
      
      ! handler to newly defined variable
      type(adios2_variable), intent(out):: variable
      
      ! io component owning the variable
      type(adios2_io), intent(in):: io
      
      ! unique variable identifier within io
      character*(*), intent(in):: variable_name
      
      ! defines variable type from adios2 parameters, see next 
      integer, intent(in):: adios2_type 
      
      ! number of dimensions
      integer, value, intent(in):: ndims
      
      ! variable shape, global size, dimensions
      ! to create local variables optional pass adios2_null_dims 
      integer(kind=8), dimension(:), intent(in):: shape_dims
      
      ! variable start, local offset, dimensions
      ! to create local variables optional pass adios2_null_dims 
      integer(kind=8), dimension(:), intent(in):: start_dims
      
      ! variable count, local size, dimensions
      integer(kind=8), dimension(:), intent(in):: count_dims
      
      ! .true. : constant dimensions, shape, start and count won't change 
      !          (mesh sizes, number of nodes)
      !          adios2_constant_dims = .true. use for code clarity
      ! .false. : variable dimensions, shape, start and count could change
      !           (number of particles)
      !           adios2_variable_dims = .false. use for code clarity
      logical, value, intent(in):: adios2_constant_dims
      
   
* available :f90:`adios2_type` parameters in :f90:`subroutine adios2_define_variable` 
   
   .. code-block:: fortran
      
      integer, parameter :: adios2_type_character = 0
      integer, parameter :: adios2_type_real = 2
      integer, parameter :: adios2_type_dp = 3
      integer, parameter :: adios2_type_complex = 4
      integer, parameter :: adios2_type_complex_dp = 5
      
      integer, parameter :: adios2_type_integer1 = 6
      integer, parameter :: adios2_type_integer2 = 7
      integer, parameter :: adios2_type_integer4 = 8
      integer, parameter :: adios2_type_integer8 = 9
      
      integer, parameter :: adios2_type_string = 10
      integer, parameter :: adios2_type_string_array = 11
  

.. tip::

   Always prefer using ``adios2_type_xxx`` parameters explicitly rather than raw numbers. 
   *e.g.* use ``adios2_type_dp`` instead of ``3``
  
  
               
* :f90:`subroutine adios2_define_attribute`
   
   .. code-block:: fortran

      ! Single value attributes
      subroutine adios2_define_attribute(attribute, io, attribute_name, data, ierr)
                                         
      ! 1D array attributes
      subroutine adios2_define_attribute(attribute, io, attribute_name, data, elements, ierr)
         
      ! WHERE:
      
      ! handler to newly defined attribute
      type(adios2_attribute), intent(out):: attribute 
      
      ! io component owning the attribute
      type(adios2_io), intent(in):: io
      
      ! unique attribute identifier within io
      character*(*), intent(in):: attribute_name
      
      ! overloaded subroutine allows for multiple attribute data types
      ! they can be single values or 1D arrays
      Generic Fortran types, intent(in):: data 
      Generic Fortran types, dimension(:), intent(in):: data
                                        
      ! number of elements if passing a 1D array in data argument
      integer, intent(in):: elements


* :f90:`subroutine adios2_set_engine` set engine type in code, see :ref:`Supported Engines` for a list of available engines
   
   .. code-block:: fortran
      
      subroutine adios2_set_engine(io, engine_type, ierr)
      
      ! WHERE:
      
      ! io component owning the attribute
      type(adios2_io), intent(in):: io
      
      ! engine_type: BPFile (default), HDF5, DataMan, SST, InSituMPI
      character*(*), intent(in):: engine_type

* :f90:`subroutine adios2_set_parameter` set IO key/value pair parameter in code, see :ref:`Supported Engines` for a list of available parameters for each engine type
   
   .. code-block:: fortran
      
      subroutine adios2_set_parameter(io, key, value, ierr)
      
      ! WHERE:
      
      ! io component owning the attribute
      type(adios2_io), intent(in):: io
      
      ! key in the key/value pair parameter
      character*(*), intent(in):: key
      
      ! value in the key/value pair parameter
      character*(*), intent(in):: value
      
      
* :f90:`subroutine adios2_inquire_variable` inquire for existing variable by its unique name
   
   .. code-block:: fortran
   
      subroutine adios2_inquire_variable(variable, io, name, ierr)
        
      ! WHERE:
      
      ! output variable handler:
      ! variable%valid = .true. points to valid found variable
      ! variable%valid = .false. variable not found
      type(adios2_variable), intent(out) :: variable
      
      ! io in which search for variable is performed
      type(adios2_io), intent(in) :: io
      
      ! unique key name to search for variable 
      character*(*), intent(in) :: name
      
* :f90:`subroutine adios2_inquire_attribute` inquire for existing attribute by its unique name
   
   .. code-block:: fortran
   
      subroutine adios2_inquire_attribute(attribute, io, name, ierr)
        
      ! WHERE:
      
      ! output attribute handler:
      ! attribute%valid = .true. points to valid found attribute
      ! attribute%valid = .false. attribute not found
      type(adios2_attribute), intent(out) :: attribute
      
      ! io in which search for attribute is performed
      type(adios2_io), intent(in) :: io
      
      ! unique key name to search for attribute 
      character*(*), intent(in) :: name

..  caution::

   Use the ``adios2_remove_*`` subroutines with extreme CAUTION.
   They create outdated dangling information in the ``adios2_type`` handlers.
   If you don't need them, don't use them. 


* :f90:`subroutine adios2_remove_variable` remove existing variable by its unique name
   
   .. code-block:: fortran
   
      subroutine adios2_remove_variable(io, name, result, ierr)
        
      ! WHERE:
      
      ! io in which search and removal for variable is performed
      type(adios2_io), intent(in) :: io
      
      ! unique key name to search for variable 
      character*(*), intent(in) :: name
      
      ! true: variable removed, false: variable not found, not removed
      logical, intent(out) :: result
      
      
* :f90:`subroutine adios2_remove_attribute` remove existing attribute by its unique name
   
   .. code-block:: fortran
   
      subroutine adios2_remove_attribute(io, name, result, ierr)
        
      ! WHERE:
      
      ! io in which search and removal for attribute is performed
      type(adios2_io), intent(in) :: io
      
      ! unique key name to search for attribute 
      character*(*), intent(in) :: name
      
      ! true: attribute removed, false: attribute not found, not removed
      logical, intent(out) :: result
      
* :f90:`subroutine adios2_remove_all_variables` remove all existing variables
   
   .. code-block:: fortran
   
      subroutine adios2_remove_variable(io, ierr)
        
      ! WHERE:
      
      ! io in which search and removal for all variables is performed
      type(adios2_io), intent(in) :: io
      
      
* :f90:`subroutine adios2_remove_all_attributes` remove all existing attributes
   
   .. code-block:: fortran
   
      subroutine adios2_remove_all_attributes(io, ierr)
        
      ! WHERE:
      
      ! io in which search and removal for all attributes is performed
      type(adios2_io), intent(in) :: io

* :f90:`subroutine adios2_flush_all_engines` flushes all existing engines opened by this io
   
   .. code-block:: fortran
   
      subroutine adios2_flush_all_engines(io, ierr)
        
      ! WHERE:
      
      ! io in which search and flush for all engines is performed
      type(adios2_io), intent(in) :: io 
      
* :f90:`subroutine adios2_open` opens an engine to executes IO tasks 
   
   .. code-block:: fortran
   
      ! MPI version: duplicates communicator from adios2_init
      ! Non-MPI serial version  
      subroutine adios2_open(engine, io, name, adios2_mode, ierr)
      
      ! MPI version only to pass a communicator other than the one from adios_init 
      subroutine adios2_open(engine, io, name, adios2_mode, comm, ierr)
      
      ! WHERE:
      
      ! handler to newly opened adios2 engine
      type(adios2_engine), intent(out) :: engine
      
      ! io that spawns an engine based on its configuration
      type(adios2_io), intent(in) :: io
      
      ! unique engine identifier within io, file name for default bpfile engine 
      character*(*), intent(in) :: name
      
      ! Optional MPI communicator, only in MPI library
      integer, intent(in) :: comm
      
      ! open mode parameter: 
      !                      adios2_mode_write,
      !                      adios2_mode_append,
      !                      adios2_mode_read,  
      integer, intent(in):: adios2_mode






:ref:`Variable` subroutines
---------------------------

* :f90:`subroutine adios2_set_shape` set new ``shape_dims`` if dims are variable in ``adios2_define_variable``
   
   .. code-block:: fortran
   
      subroutine adios2_set_selection(variable, ndims, shape_dims, ierr)
      
      ! WHERE
      
      ! variable handler
      type(adios2_variable), intent(in) :: variable
      
      ! number of dimensions in shape_dims
      integer, intent(in) :: ndims
      
      ! new shape_dims
      integer(kind=8), dimension(:), intent(in):: shape_dims



* :f90:`subroutine adios2_set_selection` set new start_dims and count_dims
   
   .. code-block:: fortran
   
      subroutine adios2_set_selection(variable, ndims, start_dims, count_dims, ierr)
      
      ! WHERE
      
      ! variable handler
      type(adios2_variable), intent(in) :: variable
      
      ! number of dimensions in start_dims and count_dims
      integer, intent(in) :: ndims
      
      ! new start_dims
      integer(kind=8), dimension(:), intent(in):: start_dims
      
      ! new count_dims
      integer(kind=8), dimension(:), intent(in):: count_dims
      
* :f90:`subroutine adios2_set_steps_selection` set new step_start and step_count
   
   .. code-block:: fortran
   
      subroutine adios2_set_selection(variable, ndims, start_dims, count_dims, ierr)
      
      ! WHERE
      
      ! variable handler
      type(adios2_variable), intent(in) :: variable
      
      ! number of dimensions in start_dims and count_dims
      integer, intent(in):: ndims
      
      ! new step_start 
      integer(kind=8), intent(in):: step_start
      
      ! new step_count (or number of steps to read from step_start)
      integer(kind=8), intent(in):: step_count
      
      
:ref:`Engine` subroutines
-------------------------

* :f90:`subroutine adios2_begin_step` moves to next step, starts at 0
   
   .. code-block:: fortran
   
      ! Full signature
      subroutine adios2_begin_step(engine, adios2_step_mode, timeout_seconds, status, ierr)
      ! Default Timeout = -1.    (block until step available)
      subroutine adios2_begin_step(engine, adios2_step_mode, ierr)
      ! Default step_mode for read and write
      subroutine adios2_begin_step(engine, ierr)
      
      ! WHERE
      
      ! engine handler
      type(adios2_engine), intent(in) :: engine
      
      ! step_mode parameter:
      !                      adios2_step_mode_read (read mode default)
      !                      adios2_step_mode_append (write mode default)
      integer, intent(in):: adios2_step_mode
      
      ! optional 
      ! engine timeout (if supported), in seconds
      real, intent(in):: timeout_seconds
      ! status of the stream from adios2_step_status_* parameters
      integer, intent(out):: status
   
   
* :f90:`subroutine adios2_current_step` extracts current step
   
   .. code-block:: fortran
   
      ! Full signature
      subroutine adios2_current_step(current_step, engine, ierr)
      
      ! WHERE:
      ! engine handler  
      type(adios2_engine), intent(in) :: engine
      
      ! populated with current_step value
      integer(kind=8), intent(out) :: current_step 

* :f90:`subroutine adios2_steps` Inspect total number of available steps, 
      use for file engines in read mode only
   
   .. code-block:: fortran
   
      ! Full signature
      subroutine adios2_steps(steps, engine, ierr)
      
      ! WHERE:
      ! engine handler  
      type(adios2_engine), intent(in) :: engine
      
      ! populated with steps value
      integer(kind=8), intent(out) :: steps 

      
* :f90:`subroutine adios2_end_step` ends current step and default behavior is to execute transport IO (flush or read). 
   
   .. code-block:: fortran
   
      ! Full signature
      subroutine adios2_end_step(engine, ierr)
      
      ! WHERE:
      ! engine handler  
      type(adios2_engine), intent(in) :: engine
   
* :f90:`subroutine adios2_put` put variable metadata and data into adios2 for IO operations. Default is deferred mode, optional sync mode, see :ref:`Put: modes and memory contracts`. Variable and data types must match.

   
   .. code-block:: fortran
   
      ! Full signature
      subroutine adios2_put(engine, variable, data, adios2_mode, ierr)
      
      ! Default adios2_mode_deferred
      subroutine adios2_put(engine, variable, data, ierr)
      
      ! WHERE:
      
      ! engine handler  
      type(adios2_engine), intent(in) :: engine
      
      ! variable handler containing metadata information  
      type(adios2_variable), intent(in) :: variable
      
      ! Fortran bindings supports data types from adios2_type in variables, 
      ! up to 6 dimensions 
      ! Generic Fortran type from adios2_type
      Generic Fortran types, intent(in):: data 
      Generic Fortran types, dimension(:), intent(in):: data
      Generic Fortran types, dimension(:,:), intent(in):: data
      Generic Fortran types, dimension(:,:,:), intent(in):: data
      Generic Fortran types, dimension(:,:,:,:), intent(in):: data
      Generic Fortran types, dimension(:,:,:,:,:), intent(in):: data
      Generic Fortran types, dimension(:,:,:,:,:,:), intent(in):: data
      
      ! mode:
      ! adios2_mode_deferred: won't execute until adios2_end_step, adios2_perform_puts or adios2_close
      ! adios2_mode_sync: special case, put data immediately, can be reused after this call
      integer, intent(in):: adios2_mode
      
      
* :f90:`subroutine adios2_perform_puts` executes deferred calls to adios2_put
      
   .. code-block:: fortran
   
      ! Full signature
      subroutine adios2_perform_puts(engine, ierr)
      
      ! WHERE:
      
      ! engine handler  
      type(adios2_engine), intent(in) :: engine
      
      
* :f90:`subroutine adios2_get` get variable data into adios2 for IO operations. Default is deferred mode, optional sync mode, see :ref:`Get: modes and memory contracts`. Variable and data types must match, variable can be obtained from ``adios2_inquire_variable``. Data must be pre-allocated.

   
   .. code-block:: fortran
   
      ! Full signature
      subroutine adios2_get(engine, variable, data, adios2_mode, ierr)
      
      ! Default adios2_mode_deferred
      subroutine adios2_get(engine, variable, data, ierr)
      
      ! WHERE:
      
      ! engine handler  
      type(adios2_engine), intent(in) :: engine
      
      ! variable handler containing metadata information  
      type(adios2_variable), intent(in) :: variable
      
      ! Fortran bindings supports data types from adios2_type in variables, 
      ! up to 6 dimensions. Must be pre-allocated 
      ! Generic Fortran type from adios2_type
      Generic Fortran types, intent(out):: data 
      Generic Fortran types, dimension(:), intent(out):: data
      Generic Fortran types, dimension(:,:), intent(out):: data
      Generic Fortran types, dimension(:,:,:), intent(out):: data
      Generic Fortran types, dimension(:,:,:,:), intent(out):: data
      Generic Fortran types, dimension(:,:,:,:,:), intent(out):: data
      Generic Fortran types, dimension(:,:,:,:,:,:), intent(out):: data
      
      ! mode:
      ! adios2_mode_deferred: won't execute until adios2_end_step, adios2_perform_gets or adios2_close
      ! adios2_mode_sync: special case, get data immediately, can be reused after this call
      integer, intent(in):: adios2_mode
      
      
* :f90:`subroutine adios2_perform_gets` executes deferred calls to ``adios2_get``
      
   .. code-block:: fortran
   
      ! Full signature
      subroutine adios2_perform_gets(engine, ierr)
      
      ! WHERE:
      
      ! engine handler  
      type(adios2_engine), intent(in) :: engine
      
      
* :f90:`subroutine adios2_close` closes engine, can't reuse unless is opened again  
      
   .. code-block:: fortran
   
      ! Full signature
      subroutine adios2_close(engine, ierr)
      
      ! WHERE:
      
      ! engine handler  
      type(adios2_engine), intent(in) :: engine
      
      
