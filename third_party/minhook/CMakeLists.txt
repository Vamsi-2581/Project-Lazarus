# MinHook v1.3.4 (commit c3fcafdc10146beb5919319d0683e44e3c30d537), vendored
# unmodified from https://github.com/TsudaKageyu/minhook - BSD-2-Clause.
add_library(minhook STATIC
  src/buffer.c
  src/hook.c
  src/trampoline.c
  src/hde/hde64.c
)
target_include_directories(minhook PUBLIC include)
if(MSVC)
  # third party code, not ours to clean up warnings in
  target_compile_options(minhook PRIVATE /W0)
endif()
