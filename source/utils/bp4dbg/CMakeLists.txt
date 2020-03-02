install(PROGRAMS bp4dbg.py
  RENAME bp4dbg
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(
  FILES
    adios2/bp4dbg/__init__.py
    adios2/bp4dbg/data.py
    adios2/bp4dbg/utils.py
    adios2/bp4dbg/metadata.py
    adios2/bp4dbg/idxtable.py
  DESTINATION ${CMAKE_INSTALL_PYTHONDIR}/adios2/bp4dbg
)
