cmake_minimum_required(VERSION 3.20)
project(rsharp VERSION 0.2.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # lets VS Code / clangd find flags

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# ---- compiler library (everything except main, so tests can link it) ----
add_library(rsharp_core
  compiler/source.cpp
  compiler/diagnostics.cpp
  compiler/token.cpp
  compiler/lexer.cpp
)
target_include_directories(rsharp_core PUBLIC compiler)

if(MSVC)
  target_compile_options(rsharp_core PUBLIC /W4 /utf-8)
else()
  target_compile_options(rsharp_core PUBLIC -Wall -Wextra -Wpedantic)
endif()

# ---- CLI: rsharp (and rsh alias) ----
add_executable(rsharp compiler/main.cpp)
target_link_libraries(rsharp PRIVATE rsharp_core)

# `rsh` alias: same program, second name
add_custom_command(TARGET rsharp POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:rsharp>
          $<TARGET_FILE_DIR:rsharp>/rsh${CMAKE_EXECUTABLE_SUFFIX}
  COMMENT "Creating rsh alias")

# ---- tests ----
enable_testing()
add_subdirectory(tests)
