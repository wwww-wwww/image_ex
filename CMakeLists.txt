cmake_minimum_required(VERSION 3.10)
project(image_ex_nif)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $ENV{MIX_COMPILE_PATH}/../priv)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $ENV{MIX_COMPILE_PATH}/../priv)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{MIX_COMPILE_PATH}/../priv)

add_library(image_ex_nif SHARED cpp_src/image_ex_nif.cpp cpp_src/lodepng.cpp)
#target_link_libraries(test_ex_nif)
target_include_directories(image_ex_nif PRIVATE
  $ENV{ERTS_INCLUDE_DIR}
  msf_gif
)
