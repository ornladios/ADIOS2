macro(EXEC CMD)
    execute_process(COMMAND "rm" "-f" ${CMD} RESULT_VARIABLE CMD_RESULT)
endmacro()

macro(EXEC_CHECK CMD)
    execute_process(COMMAND "mpirun" "-n" "8" ${CMD} RESULT_VARIABLE CMD_RESULT)
    if(CMD_RESULT)
        message(FATAL_ERROR "Error running ${CMD}")
    endif()
endmacro()

exec(${CMD1})
exec_check(${CMD2})
