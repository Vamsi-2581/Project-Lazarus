cmake_minimum_required(VERSION 3.20)
project(ProjectLazarus VERSION 0.1.0 LANGUAGES C CXX ASM_MASM)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Project Lazarus targets 64-bit Windows only (the game is x64 Windows).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# static CRT so the proxy doesn't depend on whatever VC++ redist the player has
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
  add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/W4;/permissive-;/utf-8>")
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

enable_testing()

add_subdirectory(third_party/minhook)
add_subdirectory(src/core)
add_subdirectory(src/proxy)
add_subdirectory(tests)
