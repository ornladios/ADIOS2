!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_fread_mod.f90 : ADIOS2 Fortran bindings for File high-level
!                                Write functions
!
!   Created on: Feb 28, 2018
!       Author: William F Godoy godoywf@ornl.gov
!

module adios2_fread_mod
    use adios2_parameters_mod
    use adios2_functions_mod
    implicit none

    interface adios2_fread

        ! Single Value
        module procedure adios2_fread_real
        module procedure adios2_fread_dp
        module procedure adios2_fread_complex
        module procedure adios2_fread_complex_dp
        module procedure adios2_fread_integer1
        module procedure adios2_fread_integer2
        module procedure adios2_fread_integer4
        module procedure adios2_fread_integer8

        ! 1D Array
        module procedure adios2_fread_real_1d
        module procedure adios2_fread_dp_1d
        module procedure adios2_fread_complex_1d
        module procedure adios2_fread_complex_dp_1d
        module procedure adios2_fread_integer1_1d
        module procedure adios2_fread_integer2_1d
        module procedure adios2_fread_integer4_1d
        module procedure adios2_fread_integer8_1d

        ! 2D Array
        module procedure adios2_fread_real_2d
        module procedure adios2_fread_dp_2d
        module procedure adios2_fread_complex_2d
        module procedure adios2_fread_complex_dp_2d
        module procedure adios2_fread_integer1_2d
        module procedure adios2_fread_integer2_2d
        module procedure adios2_fread_integer4_2d
        module procedure adios2_fread_integer8_2d

        ! 3D Array
        module procedure adios2_fread_real_3d
        module procedure adios2_fread_dp_3d
        module procedure adios2_fread_complex_3d
        module procedure adios2_fread_complex_dp_3d
        module procedure adios2_fread_integer1_3d
        module procedure adios2_fread_integer2_3d
        module procedure adios2_fread_integer4_3d
        module procedure adios2_fread_integer8_3d

        ! 4D Array
        module procedure adios2_fread_real_4d
        module procedure adios2_fread_dp_4d
        module procedure adios2_fread_complex_4d
        module procedure adios2_fread_complex_dp_4d
        module procedure adios2_fread_integer1_4d
        module procedure adios2_fread_integer2_4d
        module procedure adios2_fread_integer4_4d
        module procedure adios2_fread_integer8_4d

        ! 5D Array
        module procedure adios2_fread_real_5d
        module procedure adios2_fread_dp_5d
        module procedure adios2_fread_complex_5d
        module procedure adios2_fread_complex_dp_5d
        module procedure adios2_fread_integer1_5d
        module procedure adios2_fread_integer2_5d
        module procedure adios2_fread_integer4_5d
        module procedure adios2_fread_integer8_5d

        ! 6D Array
        module procedure adios2_fread_real_6d
        module procedure adios2_fread_dp_6d
        module procedure adios2_fread_complex_6d
        module procedure adios2_fread_complex_dp_6d
        module procedure adios2_fread_integer1_6d
        module procedure adios2_fread_integer2_6d
        module procedure adios2_fread_integer4_6d
        module procedure adios2_fread_integer8_6d

        ! Single Value
        module procedure adios2_fread_steps_real
        module procedure adios2_fread_steps_dp
        module procedure adios2_fread_steps_complex
        module procedure adios2_fread_steps_complex_dp
        module procedure adios2_fread_steps_integer1
        module procedure adios2_fread_steps_integer2
        module procedure adios2_fread_steps_integer4
        module procedure adios2_fread_steps_integer8

        ! 1D Array
        module procedure adios2_fread_steps_real_1d
        module procedure adios2_fread_steps_dp_1d
        module procedure adios2_fread_steps_complex_1d
        module procedure adios2_fread_steps_complex_dp_1d
        module procedure adios2_fread_steps_integer1_1d
        module procedure adios2_fread_steps_integer2_1d
        module procedure adios2_fread_steps_integer4_1d
        module procedure adios2_fread_steps_integer8_1d

        ! 2D Array
        module procedure adios2_fread_steps_real_2d
        module procedure adios2_fread_steps_dp_2d
        module procedure adios2_fread_steps_complex_2d
        module procedure adios2_fread_steps_complex_dp_2d
        module procedure adios2_fread_steps_integer1_2d
        module procedure adios2_fread_steps_integer2_2d
        module procedure adios2_fread_steps_integer4_2d
        module procedure adios2_fread_steps_integer8_2d

        ! 3D Array
        module procedure adios2_fread_steps_real_3d
        module procedure adios2_fread_steps_dp_3d
        module procedure adios2_fread_steps_complex_3d
        module procedure adios2_fread_steps_complex_dp_3d
        module procedure adios2_fread_steps_integer1_3d
        module procedure adios2_fread_steps_integer2_3d
        module procedure adios2_fread_steps_integer4_3d
        module procedure adios2_fread_steps_integer8_3d

        ! 4D Array
        module procedure adios2_fread_steps_real_4d
        module procedure adios2_fread_steps_dp_4d
        module procedure adios2_fread_steps_complex_4d
        module procedure adios2_fread_steps_complex_dp_4d
        module procedure adios2_fread_steps_integer1_4d
        module procedure adios2_fread_steps_integer2_4d
        module procedure adios2_fread_steps_integer4_4d
        module procedure adios2_fread_steps_integer8_4d

        ! 5D Array
        module procedure adios2_fread_steps_real_5d
        module procedure adios2_fread_steps_dp_5d
        module procedure adios2_fread_steps_complex_5d
        module procedure adios2_fread_steps_complex_dp_5d
        module procedure adios2_fread_steps_integer1_5d
        module procedure adios2_fread_steps_integer2_5d
        module procedure adios2_fread_steps_integer4_5d
        module procedure adios2_fread_steps_integer8_5d

        ! 6D Array
        module procedure adios2_fread_steps_real_6d
        module procedure adios2_fread_steps_dp_6d
        module procedure adios2_fread_steps_complex_6d
        module procedure adios2_fread_steps_complex_dp_6d
        module procedure adios2_fread_steps_integer1_6d
        module procedure adios2_fread_steps_integer2_6d
        module procedure adios2_fread_steps_integer4_6d
        module procedure adios2_fread_steps_integer8_6d

    end interface

contains

    include 'contains/adios2_fread.f90'
    include 'contains/adios2_fread_steps.f90'

end module
