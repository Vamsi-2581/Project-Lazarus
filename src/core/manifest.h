#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "core/ini.h"

namespace lz {

// Parsed mods/<id>/mod.ini. Example:
//
//   [mod]
//   id = graphics-settings
//   name = Graphics Settings
//   version = 1.0.0
//   author = Project Lazarus
//   dll = graphics_settings.dll
//   api_version = 1
//   game_builds = any           ; or comma-separated build fingerprints
//   requires =                  ; other mod ids that must load first
struct ModManifest {
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string dll;
    int api_version = 0;
    std::vector<std::string> game_builds;  // empty means "any"
    std::vector<std::string> requires_;    // trailing _ because 'requires' is a C++20 keyword
    std::filesystem::path dir;

    bool supports_any_build() const { return game_builds.empty(); }
};

bool is_valid_mod_id(const std::string& id);

// Fills `out` from the ini. Returns false and pushes reasons into `errors` if the
// manifest is unusable. Soft problems (unknown keys) go into `warnings`.
bool parse_manifest(const Ini& ini, const std::filesystem::path& dir, ModManifest& out,
                    std::vector<std::string>& errors, std::vector<std::string>& warnings);

}  // namespace lz
