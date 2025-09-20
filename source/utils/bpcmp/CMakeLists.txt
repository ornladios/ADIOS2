configure_file(bpcmp.py "${PROJECT_BINARY_DIR}/bin/bpcmp.py" COPYONLY)

install(PROGRAMS bpcmp.py
  DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_scripts-runtime
  ${ADIOS2_MAYBE_EXCLUDE_FROM_ALL}
)
