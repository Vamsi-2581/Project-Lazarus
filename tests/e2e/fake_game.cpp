// The "game exe": imports UnityPlayer.dll and calls into it, nothing else.
// Same shape as The Walking Dead Destinies.exe, which is a tiny stub that
// only imports UnityPlayer.dll + KERNEL32.

#include <windows.h>

extern "C" __declspec(dllimport) int fake_unity_main(const wchar_t* report_path);

int wmain(int argc, wchar_t** argv) {
    if (argc < 2) return 99;
    return fake_unity_main(argv[1]);
}
