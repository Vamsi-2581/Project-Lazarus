add_library(hello_mod SHARED hello_mod.cpp)
target_include_directories(hello_mod PRIVATE ${PROJECT_SOURCE_DIR}/include)
# not part of dist on purpose - examples aren't shipped to players
