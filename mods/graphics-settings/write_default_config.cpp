// build helper: writes kDefaultConfig to argv[1]
#include <cstdio>

#include "settings.h"

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    FILE* f = std::fopen(argv[1], "wb");
    if (!f) return 1;
    std::fputs(gfx::kDefaultConfig, f);
    return std::fclose(f) == 0 ? 0 : 1;
}
