# CMake toolchain file for MemorySanitizer builds
#
# This configures clang to use MSan with origin tracking and links against
# the MSan-instrumented libc++ in /opt/msan.

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(_msan_flags "-fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer")
set(_cxx_flags "-nostdinc++ -isystem /opt/msan/include -isystem /opt/msan/include/c++/v1 ${_msan_flags}")
set(_link_flags "-stdlib=libc++ -L/opt/msan/lib -lc++abi -Wl,-rpath,/opt/msan/lib -Wno-unused-command-line-argument ${_msan_flags}")

set(CMAKE_C_FLAGS_INIT "${_msan_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_cxx_flags}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_link_flags}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_link_flags}")

set(CMAKE_PREFIX_PATH /opt/msan)
