add_library(fps_counter SHARED fps_counter.cpp)
target_include_directories(fps_counter PRIVATE ${PROJECT_SOURCE_DIR}/include)
# d3d11/d3d12/dxgi are loaded at runtime, only need the headers + uuids
target_link_libraries(fps_counter PRIVATE dxguid)

set(FPS_OUT ${CMAKE_BINARY_DIR}/dist/Lazarus/mods/fps-counter)
set_target_properties(fps_counter PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:${FPS_OUT}>)
add_custom_command(TARGET fps_counter POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/mod.ini ${FPS_OUT}/mod.ini
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/config.ini ${FPS_OUT}/config.ini)
