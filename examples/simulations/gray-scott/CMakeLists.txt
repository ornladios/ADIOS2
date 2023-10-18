#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.12)
project(ADIOS2SimulationsGrayScottExample)

if(NOT TARGET adios2_core)
  set(_components CXX)

  find_package(MPI COMPONENTS C)
  if(MPI_FOUND)
    # Workaround for various MPI implementations forcing the link of C++ bindings
    add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)

    list(APPEND _components MPI)
  endif()

  find_package(ADIOS2 REQUIRED COMPONENTS ${_components})
endif()

if(ADIOS2_HAVE_MPI)
  add_executable(adios2_simulations_gray-scott
    simulation/main.cpp
    simulation/gray-scott.cpp
    simulation/settings.cpp
    simulation/writer.cpp
    simulation/restart.cpp
  )
  target_link_libraries(adios2_simulations_gray-scott adios2::cxx11_mpi MPI::MPI_C)
  install(TARGETS adios2_simulations_gray-scott RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  add_executable(adios2_simulations_gray-scott_pdf-calc analysis/pdf-calc.cpp)
  target_link_libraries(adios2_simulations_gray-scott_pdf-calc adios2::cxx11_mpi MPI::MPI_C)
  install(TARGETS adios2_simulations_gray-scott_pdf-calc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  install(FILES "adios2.xml" "adios2-inline-plugin.xml"
    "visit-bp4.session" "visit-bp4.session.gui"
    "visit-sst.session" "visit-sst.session.gui"
    "simulation/settings-files.json"
    "simulation/settings-staging.json"
    "simulation/settings-inline.json"
    "plot/decomp.py" "plot/gsplot.py" "plot/pdfplot.py"
    "ReadMe.md"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/adios2/examples/simulations/gray-scott)

  install(DIRECTORY "catalyst" DESTINATION ${CMAKE_INSTALL_PREFIX}/share/adios2/examples/simulations/gray-scott)

  install(PROGRAMS "cleanup.sh" DESTINATION ${CMAKE_INSTALL_PREFIX}/share/adios2/examples/simulations/gray-scott)

  find_package(VTK QUIET)
  if(VTK_FOUND)
    add_executable(adios2_simulations_gray-scott_curvature analysis/curvature.cpp)
    target_link_libraries(adios2_simulations_gray-scott_curvature adios2::cxx11_mpi MPI::MPI_C ${VTK_LIBRARIES})
    install(TARGETS adios2_simulations_gray-scott_curvature RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    add_executable(adios2_simulations_gray-scott_find-blobs analysis/find_blobs.cpp)
    target_link_libraries(adios2_simulations_gray-scott_find-blobs adios2::cxx11_mpi MPI::MPI_C ${VTK_LIBRARIES})
    install(TARGETS adios2_simulations_gray-scott_find-blobs RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    add_executable(adios2_simulations_gray-scott_isosurface analysis/isosurface.cpp)
    target_link_libraries(adios2_simulations_gray-scott_isosurface adios2::cxx11_mpi MPI::MPI_C ${VTK_LIBRARIES})
    install(TARGETS adios2_simulations_gray-scott_isosurface RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # add_executable(adios2_simulations_gray-scott_render-isosurface plot/render_isosurface.cpp)
    # target_link_libraries(adios2_simulations_gray-scott_render-isosurface adios2::cxx11_mpi MPI::MPI_C ${VTK_LIBRARIES})
  endif()
endif()
