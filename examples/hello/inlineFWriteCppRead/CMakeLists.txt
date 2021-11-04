#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(FortranCInterface)
FortranCInterface_HEADER(InlineExampleFC.h MACRO_NAMESPACE "FC_")
FortranCInterface_VERIFY(CXX QUIET)

add_library(InlineMixedLangCPP inlineMixedLang.cpp)
target_link_libraries(InlineMixedLangCPP adios2::cxx11)
target_include_directories(InlineMixedLangCPP
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    )

add_executable(InlineMixedLangF inlineMixedLang.f90)
target_link_libraries(InlineMixedLangF adios2::fortran InlineMixedLangCPP)
set_target_properties(InlineMixedLangF PROPERTIES LINKER_LANGUAGE Fortran)
