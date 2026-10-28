# The proxy has to be called version.dll for the game to pick it up, but the
# CMake target is named differently so nothing accidentally links against our
# import lib thinking it's the Windows one.
add_library(lazarus_proxy SHARED
  dllmain.cpp
  iat.cpp
  runtime.cpp
  version_forward.asm
  version.def
)
set_target_properties(lazarus_proxy PROPERTIES OUTPUT_NAME version)
target_include_directories(lazarus_proxy PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include)
target_compile_definitions(lazarus_proxy PRIVATE LAZARUS_VERSION="${PROJECT_VERSION}")
target_link_libraries(lazarus_proxy PRIVATE lazarus_core minhook)
