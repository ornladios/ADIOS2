****************
Fortran bindings
****************

.. role:: fortran(code)
   :language: fortran
   :class: highlight

The Fortran bindings API consist entirely on subroutines calls. Always, the 1st argument is a type (Fortran struct) to one of the ADIOS2 components (adios2_*), while the last argument is an error integer flag, :fortran:`integer ierr`.
Typically: 

   .. code-block:: fortran

      ! 0: no error
      ! 1: an error occured
      integer, intent(out):: ierr 

The following bullets describe the overall component representation and the main subroutines and available overloaded versions in the API.

* **ADIOS2 typed handlers**: ADIOS2 Fortran handlers are mapped 1-to-1 to the ADIOS components described in the :ref:`Application Programmer Interface` section
 
   .. code-block:: fortran

      type(adios2_adios) :: adios
      type(adios2_io) :: io
      type(adios2_variable) :: variable
      type(adios2_attribute) :: attribute
      type(adios2_engine) :: engine
   

* :ref:`ADIOS` component subroutines:

   * :fortran:`subroutine adios2_init` 

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
      
      ! adios handler owning adios2_io
      type(adios2_adios), intent(out):: adios 
      
      ! MPI Communicator
      integer, intent(in):: comm 
      
      ! Optional runtime configuration file (*.xml), see Runtime Configuration Files
      character*(*), intent(in) :: config_file
      
      ! .true. (adios2_debug_mode_on): enable extra user input checks-> recommended
      ! .false. (adios2_debug_mode_of): disable extra user input checks
      logical, intent(in):: adios2_debug_mode
      

   * :fortran:`subroutine adios2_declare_io`

   .. code-block:: fortran

      subroutine adios2_declare_io(io, adios, io_name, ierr)
      
      ! adios component from adios2_init spawning io tasks 
      type(adios2_adios), intent(in):: adios
      
      ! io component that defines an IO tasks inside adios component
      type(adios2_io), intent(out):: io
      
      ! unique name associated with this io component inside adios
      character*(*), intent(in):: io

      
* :ref:`IO` component subroutines:    
      
   * :fortran:`subroutine adios2_define_variable` 

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
      
      ! unique variable identifier
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
         
   
   * available :fortran:`adios2_type` parameters in :fortran:`subroutine adios2_define_variable` 
   
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
   

   * :fortran:`subroutine adios2_define_attribute` 
   
   .. code-block:: fortran

      ! Single value attributes
      subroutine adios2_define_attribute(attribute, io, attribute_name, data, ierr)
                                         
      ! 1D array attributes
      subroutine adios2_define_attribute(attribute, io, attribute_name, data, elements, ierr)
                                          
      