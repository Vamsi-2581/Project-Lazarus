#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "core/ini.h"
#include "core/log.h"

namespace lz {

// Lazarus/lazarus.ini. Every field has a safe default so a missing or broken
// file means "loader runs, loads nothing it wasn't told to".
struct LoaderConfig {
    bool enabled = true;          // master switch. false = proxy just forwards version.dll and does nothing else
    bool safe_mode = false;       // load no mods at all
    LogLevel log_level = LogLevel::Info;
    bool ignore_build_check = false;  // load build-pinned mods on unknown game builds anyway
    std::vector<std::string> enabled_mods;  // explicit opt-in list, in user's order
    std::vector<std::string> warnings;      // anything odd found while reading
};

// Never fails: missing file -> defaults + warning. That's intentional, a typo
// in lazarus.ini must not stop the game from launching.
LoaderConfig load_config(const std::filesystem::path& path);
LoaderConfig config_from_ini(const Ini& ini);

// what the installer drops in on first install
extern const char* const kDefaultConfigText;

}  // namespace lz
