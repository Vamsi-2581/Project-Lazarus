#pragma once
#include <windows.h>

namespace lzrt {

void set_proxy_module(HMODULE self);

// Called from DllMain / the GetProcAddress hook, i.e. possibly under the
// loader lock. Appends straight to Lazarus/logs/bootstrap.log with raw Win32
// calls and also buffers the line for lazarus.log. No allocations beyond a
// fixed buffer.
void bootstrap_note(const char* msg);

// main thread, right after the real il2cpp_init returned
void on_il2cpp_ready(HMODULE game_assembly);
// main thread, right before the real il2cpp_shutdown runs
void on_il2cpp_shutdown();

}  // namespace lzrt
