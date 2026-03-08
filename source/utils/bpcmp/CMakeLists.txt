# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

configure_file(bpcmp.py "${PROJECT_BINARY_DIR}/bin/bpcmp.py" COPYONLY)

install(PROGRAMS bpcmp.py
  DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT adios2_scripts-runtime
  ${ADIOS2_MAYBE_EXCLUDE_FROM_ALL}
)
