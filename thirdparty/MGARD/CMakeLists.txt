message_start_thirdparty()

find_package(ZLIB)
if(ZLIB_FOUND)
  add_subdirectory(MGARD)
endif()

message_end_thirdparty()
