cmake_minimum_required(VERSION 3.10)

project(nsspl)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_compile_options(-pedantic -Wall -Wextra -Wsign-conversion
  -Wconversion -Wshadow)


include_directories(include/)

file(GLOB
  SRCS "src/*.cpp")

add_executable(${PROJECT_NAME} ${SRCS})
