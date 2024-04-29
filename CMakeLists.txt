cmake_minimum_required(VERSION 3.27)
project(reproduce_libclang_issue)

set(CMAKE_CXX_STANDARD 20)

add_executable(reproduce_libclang_issue main.cpp)

find_package(tiny-process-library CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)

target_link_libraries(
  reproduce_libclang_issue
  PRIVATE
    tiny-process-library::tiny-process-library
    libclang
)