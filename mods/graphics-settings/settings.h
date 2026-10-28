#pragma once
// Config parsing for the graphics-settings mod, kept free of any loader/game
// dependency so it can be unit tested.

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace gfx {

// UnityEngine.FullScreenMode
enum WindowMode { kLeave = -1, kExclusive = 0, kBorderless = 1, kMaximized = 2, kWindowed = 3 };

struct Settings {
    int window_mode = kLeave;
    int width = 0;  // 0 = keep whatever the game picked
    int height = 0;
    int refresh_rate = 0;

    int target_fps = 0;   // 0 = leave alone, -1 = unlimited, else cap
    int vsync = -1;       // -1 = leave alone, 0 = off, 1..4 = every Nth vblank
    int quality_level = -1;
    double lod_bias = 0;  // 0 = leave alone
    int anisotropic = -1; // -1 = leave alone, 0 disable, 1 enable, 2 force on

    bool reapply_each_scene = true;

    bool touches_display() const { return window_mode != kLeave || width > 0 || refresh_rate > 0; }
};

using Getter = std::function<std::optional<std::string>(const char* section, const char* key)>;

// never fails: bad values become "leave alone" with a warning so a typo
// can't make the game unlaunchable
Settings parse(const Getter& get, std::vector<std::string>& warnings);

const char* window_mode_name(int m);

// the config.ini shipped with the mod, with everything defaulting to "leave alone"
extern const char* const kDefaultConfig;

}  // namespace gfx
