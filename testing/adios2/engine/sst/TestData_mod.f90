! Distributed under the OSI-approved Apache License, Version 2.0.  See
! accompanying file Copyright.txt for details.
!
! SmallTestData_mod.f90 : small Fortran 90 arrays data for tests
!
!  Created on: Aug 9, 2017
!      Author: William F Godoy godoywf@ornl.gov
!

module sst_test_data
    implicit none

    integer(kind=1), dimension(10) :: data_I8 = &
                                                 (/0, 1, -2, 3, -4, 5, -6, 7, -8, 9/)

    integer(kind=2), dimension(10) :: data_I16 = &
                                                 (/512, 513, -510, 515, -508, 517, -506, 519, -504, 521/)

    integer(kind=4), dimension(10) :: data_I32 = &
                                                 (/131072, 131073, -131070, 131075, -131068, 131077, -131066, 131079, &
                                                   -131064, 131081/)

    integer(kind=8), dimension(10) :: data_I64 = &
                                                 (/8589934592_8, 8589934593_8, -8589934590_8, 8589934595_8, &
                                                   -8589934588_8, 8589934597_8, -8589934586_8, 8589934599_8, &
                                                   -8589934584_8, 8589934601_8/)

    real(kind=4), dimension(10) :: data_R32 = &
                                              (/0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1/)

    real(kind=8), dimension(10) :: data_R64 = &
                                              (/10.2D0, 11.2D0, 12.2D0, 13.2D0, 14.2D0, 15.2D0, 16.2D0, 17.2D0, 18.2D0, 19.2D0/)

    real (kind=8), dimension(8, 2) :: data_R64_2d = &
         reshape((/0D0, 1D0, 2D0, 3D0, 4D0, 5D0, 6D0, 7D0, &
         1000D0, 1001D0, 1002D0, 1003D0, 1004D0, 1005D0, 1006D0, 1007D0/),&
         (/8,2/))

    contains
    subroutine GenerateTestData(step, rank, size)
      INTEGER, INTENT(IN) :: step, rank, size

      
      integer :: j
      j = rank + 1 + step * size;
      data_I8 = data_I8 + j
      data_I16 = data_I16 + j
      data_I32 = data_I32 + j
      data_I64 = data_I64 + j
      data_R32 = data_R32 + j
      data_R64 = data_R64 + j
      data_R64_2d = data_R64_2d + j

    end subroutine GenerateTestData

  end module sst_test_data
