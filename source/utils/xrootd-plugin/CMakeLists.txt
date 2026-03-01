  # ADIOS SSI service plugin (server-side, handles SSI protocol requests)
  add_library(adios2_xrootd SHARED XrdSsiSvProvider.cpp XrdSsiSvService.cpp XrdSsiSvStreamActive.cpp XrdSsiSvStreamPassive.cpp AdiosFilePool.cpp)
  target_include_directories(adios2_xrootd PRIVATE ${PROJECT_SOURCE_DIR}/bindings/CXX ${XROOTD_INCLUDE_DIRS})
  target_link_libraries(adios2_xrootd PUBLIC adios2::cxx PRIVATE adios2_core ${XROOTD_CLIENT_LIBRARIES} ${XROOTD_UTILS_LIBRARIES} ${XROOTD_SSI_LIBRARIES})
  install(TARGETS adios2_xrootd DESTINATION ${CMAKE_INSTALL_LIBDIR})

  # HTTP-to-SSI bridge handler (enables HTTPS access to SSI services)
  # This plugin allows SSI-based services to be accessed via HTTP/HTTPS
  # Configure in xrootd.cf with: http.exthandler ssi libadios2_xrootd_http.so
  add_library(adios2_xrootd_http SHARED XrdHttpSsiHandler.cpp)
  target_include_directories(adios2_xrootd_http PRIVATE ${XROOTD_INCLUDE_DIRS})
  target_link_libraries(adios2_xrootd_http PUBLIC ${XROOTD_UTILS_LIBRARIES} ${XROOTD_SSI_LIBRARIES} ${XROOTD_HTTP_LIBRARIES} ${CMAKE_DL_LIBS})
  set_target_properties(adios2_xrootd_http PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
  )
  install(TARGETS adios2_xrootd_http DESTINATION ${CMAKE_INSTALL_LIBDIR})
