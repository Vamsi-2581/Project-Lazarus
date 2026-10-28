add_executable(lazarus_tests
  test_main.cpp
  test_ini.cpp
  test_log.cpp
  test_manifest.cpp
  test_config.cpp
  test_plan.cpp
  test_state.cpp
)
target_link_libraries(lazarus_tests PRIVATE lazarus_core)
add_test(NAME core_unit_tests COMMAND lazarus_tests)
add_subdirectory(e2e)
