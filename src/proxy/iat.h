#pragma once
#include <windows.h>

namespace lzrt {

// Swap one entry in `module`'s import address table. Matches the import by
// name via the import name table, so it works whether or not the IAT is
// already bound. Returns false if the module doesn't import that function.
bool patch_iat(HMODULE module, const char* import_dll, const char* func, void* replacement, void** original);

}  // namespace lzrt
