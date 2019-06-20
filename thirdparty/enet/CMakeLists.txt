message_start_thirdparty()

set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_CMAKEDIR}/thirdparty")
set(ENET_LIBRARY_PREFIX adios2_ CACHE INTERNAL "")
set(ENET_INSTALL_HEADERS OFF CACHE INTERNAL "")
set(ENET_INSTALL_PKGCONFIG OFF CACHE INTERNAL "")

add_subdirectory(enet)
set(enet_DIR ${CMAKE_CURRENT_BINARY_DIR}/enet CACHE INTERNAL "")

message_end_thirdparty()
