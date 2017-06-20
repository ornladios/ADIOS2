# This is an extremely simple and tiny library.  Rather than build it
# seperately we'll just create an interface library containing the sources and
# headers
add_library(pugixml INTERFACE)
target_sources(pugixml INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/pugixml/src/pugiconfig.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pugixml/src/pugixml.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pugixml/src/pugixml.cpp
)
target_include_directories(pugixml INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/pugixml/src
)
