# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

install(PROGRAMS bp4dbg.py
  RENAME bp4dbg
  DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_scripts-runtime
  ${ADIOS2_MAYBE_EXCLUDE_FROM_ALL}
)
install(
  FILES
    adios2/bp4dbg/__init__.py
    adios2/bp4dbg/data.py
    adios2/bp4dbg/utils.py
    adios2/bp4dbg/metadata.py
    adios2/bp4dbg/idxtable.py
  DESTINATION ${CMAKE_INSTALL_PYTHONDIR}/adios2/bp4dbg COMPONENT adios2_scripts-runtime
  ${ADIOS2_MAYBE_EXCLUDE_FROM_ALL}
)
