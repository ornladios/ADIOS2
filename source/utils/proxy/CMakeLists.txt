install(PROGRAMS proxy-server.py
        RENAME proxy-server
        DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_scripts-runtime
)

install(PROGRAMS remote-server.py
        RENAME remote-server
        DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_scripts-runtime
)
install(
        FILES
        adios2/proxy/proxy-server.py
        DESTINATION ${CMAKE_INSTALL_PYTHONDIR}/adios2/proxy COMPONENT adios2_scripts-runtime
)
install(
        FILES
        adios2/proxy/remote-server.py
        DESTINATION ${CMAKE_INSTALL_PYTHONDIR}/adios2/proxy COMPONENT adios2_scripts-runtime
)