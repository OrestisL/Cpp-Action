cmake_minimum_required(VERSION 3.16)
project(myproject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # clangd needs this

add_executable(csharp-events src/main.cpp)
target_include_directories(csharp-events PRIVATE ${CMAKE_SOURCE_DIR}/Headers)