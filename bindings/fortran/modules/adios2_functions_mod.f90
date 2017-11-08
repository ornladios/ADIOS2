module adios2_functions
    implicit none

contains

    integer function adios2_LogicalToInt( logical_value )
        logical, value, intent(in) :: logical_value

        adios2_LogicalToInt = 0
        if( logical_value ) then
            adios2_LogicalToInt = 1
        end if

    end function

end module
