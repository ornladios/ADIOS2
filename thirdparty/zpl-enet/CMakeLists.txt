message_start_thirdparty()

set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_CMAKEDIR}/thirdparty")
set(ENET_LIBRARY_PREFIX adios2_ CACHE INTERNAL "")
set(ENET_INSTALL_HEADERS OFF CACHE INTERNAL "")
set(ENET_INSTALL_PKGCONFIG OFF CACHE INTERNAL "")

add_subdirectory(zpl-enet)
set(enet_DIR ${CMAKE_CURRENT_BINARY_DIR}/zpl-enet CACHE INTERNAL "")
set(zpl-enet_DIR ${CMAKE_CURRENT_BINARY_DIR}/zpl-enet CACHE INTERNAL "")
set(zpl_enet_DIR ${CMAKE_CURRENT_BINARY_DIR}/zpl-enet CACHE INTERNAL "")

message_end_thirdparty()
