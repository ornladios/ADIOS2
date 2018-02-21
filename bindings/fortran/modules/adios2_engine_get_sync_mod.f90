!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  adios2_engine_get_sync_mod.f90 : ADIOS2 Fortran bindings for Engine generic
!                               Read functions
!
!   Created on: Aug 22, 2017
!       Author: William F Godoy godoywf@ornl.gov
!
module adios2_engine_get_sync

    implicit none

    interface adios2_get_sync

        ! Single Value
        module procedure adios2_get_sync_real
        module procedure adios2_get_sync_dp
        module procedure adios2_get_sync_complex
        module procedure adios2_get_sync_complex_dp
        module procedure adios2_get_sync_integer1
        module procedure adios2_get_sync_integer2
        module procedure adios2_get_sync_integer4
        module procedure adios2_get_sync_integer8

        ! 1D Array
        module procedure adios2_get_sync_real_1d
        module procedure adios2_get_sync_dp_1d
        module procedure adios2_get_sync_complex_1d
        module procedure adios2_get_sync_complex_dp_1d
        module procedure adios2_get_sync_integer1_1d
        module procedure adios2_get_sync_integer2_1d
        module procedure adios2_get_sync_integer4_1d
        module procedure adios2_get_sync_integer8_1d

        ! 2D Array
        module procedure adios2_get_sync_real_2d
        module procedure adios2_get_sync_dp_2d
        module procedure adios2_get_sync_complex_2d
        module procedure adios2_get_sync_complex_dp_2d
        module procedure adios2_get_sync_integer1_2d
        module procedure adios2_get_sync_integer2_2d
        module procedure adios2_get_sync_integer4_2d
        module procedure adios2_get_sync_integer8_2d

        ! 3D Array
        module procedure adios2_get_sync_real_3d
        module procedure adios2_get_sync_dp_3d
        module procedure adios2_get_sync_complex_3d
        module procedure adios2_get_sync_complex_dp_3d
        module procedure adios2_get_sync_integer1_3d
        module procedure adios2_get_sync_integer2_3d
        module procedure adios2_get_sync_integer4_3d
        module procedure adios2_get_sync_integer8_3d

        ! 4D Array
        module procedure adios2_get_sync_real_4d
        module procedure adios2_get_sync_dp_4d
        module procedure adios2_get_sync_complex_4d
        module procedure adios2_get_sync_complex_dp_4d
        module procedure adios2_get_sync_integer1_4d
        module procedure adios2_get_sync_integer2_4d
        module procedure adios2_get_sync_integer4_4d
        module procedure adios2_get_sync_integer8_4d

        ! 5D Array
        module procedure adios2_get_sync_real_5d
        module procedure adios2_get_sync_dp_5d
        module procedure adios2_get_sync_complex_5d
        module procedure adios2_get_sync_complex_dp_5d
        module procedure adios2_get_sync_integer1_5d
        module procedure adios2_get_sync_integer2_5d
        module procedure adios2_get_sync_integer4_5d
        module procedure adios2_get_sync_integer8_5d

        ! 6D Array
        module procedure adios2_get_sync_real_6d
        module procedure adios2_get_sync_dp_6d
        module procedure adios2_get_sync_complex_6d
        module procedure adios2_get_sync_complex_dp_6d
        module procedure adios2_get_sync_integer1_6d
        module procedure adios2_get_sync_integer2_6d
        module procedure adios2_get_sync_integer4_6d
        module procedure adios2_get_sync_integer8_6d

        ! Single Value
        module procedure adios2_get_sync_by_name_real
        module procedure adios2_get_sync_by_name_dp
        module procedure adios2_get_sync_by_name_complex
        module procedure adios2_get_sync_by_name_complex_dp
        module procedure adios2_get_sync_by_name_integer1
        module procedure adios2_get_sync_by_name_integer2
        module procedure adios2_get_sync_by_name_integer4
        module procedure adios2_get_sync_by_name_integer8

        ! 1D Array
        module procedure adios2_get_sync_by_name_real_1d
        module procedure adios2_get_sync_by_name_dp_1d
        module procedure adios2_get_sync_by_name_complex_1d
        module procedure adios2_get_sync_by_name_complex_dp_1d
        module procedure adios2_get_sync_by_name_integer1_1d
        module procedure adios2_get_sync_by_name_integer2_1d
        module procedure adios2_get_sync_by_name_integer4_1d
        module procedure adios2_get_sync_by_name_integer8_1d

        ! 2D Array
        module procedure adios2_get_sync_by_name_real_2d
        module procedure adios2_get_sync_by_name_dp_2d
        module procedure adios2_get_sync_by_name_complex_2d
        module procedure adios2_get_sync_by_name_complex_dp_2d
        module procedure adios2_get_sync_by_name_integer1_2d
        module procedure adios2_get_sync_by_name_integer2_2d
        module procedure adios2_get_sync_by_name_integer4_2d
        module procedure adios2_get_sync_by_name_integer8_2d

        ! 3D Array
        module procedure adios2_get_sync_by_name_real_3d
        module procedure adios2_get_sync_by_name_dp_3d
        module procedure adios2_get_sync_by_name_complex_3d
        module procedure adios2_get_sync_by_name_complex_dp_3d
        module procedure adios2_get_sync_by_name_integer1_3d
        module procedure adios2_get_sync_by_name_integer2_3d
        module procedure adios2_get_sync_by_name_integer4_3d
        module procedure adios2_get_sync_by_name_integer8_3d

        ! 4D Array
        module procedure adios2_get_sync_by_name_real_4d
        module procedure adios2_get_sync_by_name_dp_4d
        module procedure adios2_get_sync_by_name_complex_4d
        module procedure adios2_get_sync_by_name_complex_dp_4d
        module procedure adios2_get_sync_by_name_integer1_4d
        module procedure adios2_get_sync_by_name_integer2_4d
        module procedure adios2_get_sync_by_name_integer4_4d
        module procedure adios2_get_sync_by_name_integer8_4d

        ! 5D Array
        module procedure adios2_get_sync_by_name_real_5d
        module procedure adios2_get_sync_by_name_dp_5d
        module procedure adios2_get_sync_by_name_complex_5d
        module procedure adios2_get_sync_by_name_complex_dp_5d
        module procedure adios2_get_sync_by_name_integer1_5d
        module procedure adios2_get_sync_by_name_integer2_5d
        module procedure adios2_get_sync_by_name_integer4_5d
        module procedure adios2_get_sync_by_name_integer8_5d

        ! 6D Array
        module procedure adios2_get_sync_by_name_real_6d
        module procedure adios2_get_sync_by_name_dp_6d
        module procedure adios2_get_sync_by_name_complex_6d
        module procedure adios2_get_sync_by_name_complex_dp_6d
        module procedure adios2_get_sync_by_name_integer1_6d
        module procedure adios2_get_sync_by_name_integer2_6d
        module procedure adios2_get_sync_by_name_integer4_6d
        module procedure adios2_get_sync_by_name_integer8_6d

    end interface

contains

    include'contains/adios2_engine_get_sync.f90'
    include'contains/adios2_engine_get_sync_by_name.f90'

end module
