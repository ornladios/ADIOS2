!
! Distributed under the OSI-approved Apache License, Version 2.0.  See
!  accompanying file Copyright.txt for details.
!
!  inlineMixedLangMod.f90: interface for C functions in mixed language
!  example for the inline engine
!

module InlineMixedLangMod
    use adios2
    implicit none

    interface init_reader
        module subroutine open_reader(io)
            type(adios2_io), intent(in) :: io
        end subroutine
    end interface

    interface close_reader
        module subroutine close_reader()
        end subroutine
    end interface

    interface processing
        module subroutine analyze_data()
        end subroutine
    end interface

end module
