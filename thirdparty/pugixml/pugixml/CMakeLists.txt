add_library(pugixml_impl INTERFACE)
target_sources(pugixml_impl INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/pugiconfig.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/pugixml.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/pugixml.cpp>
)
add_library(pugixml INTERFACE)
target_include_directories(pugixml INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_link_libraries(pugixml_impl INTERFACE pugixml)
install(TARGETS pugixml_impl pugixml EXPORT adios2Exports)
