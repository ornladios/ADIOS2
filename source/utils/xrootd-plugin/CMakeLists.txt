  add_library(adios2_xrootd SHARED XrdSsiSvProvider.cpp XrdSsiSvService.cpp XrdSsiSvStreamActive.cpp XrdSsiSvStreamPassive.cpp AdiosFilePool.cpp)
  target_include_directories(adios2_xrootd PRIVATE ${PROJECT_SOURCE_DIR}/bindings/CXX ${XROOTD_INCLUDE_DIRS})
  target_link_libraries(adios2_xrootd PUBLIC adios2::cxx ${XROOTD_CLIENT_LIBRARIES} ${XROOTD_UTILS_LIBRARIES} ${XROOTD_SSI_LIBRARIES})
  install(TARGETS adios2_xrootd DESTINATION ${CMAKE_INSTALL_LIBDIR})
