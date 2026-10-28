#pragma once
// Turns "what's in the mods folder" + "what the user enabled" into an ordered
// list of mods to actually load, and a list of everything skipped with a
// human readable reason. The reasons end up in the log verbatim, so they're
// written for players, not for us.

#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "core/config.h"
#include "core/manifest.h"

namespace lz {

struct DiscoveredMod {
    std::string folder;  // directory name, used as id fallback in messages
    ModManifest manifest;
    bool valid = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

// Scans <mods_dir>/*/mod.ini. Folders without mod.ini are ignored with a
// warning. Sorted by folder name so results don't depend on filesystem order.
std::vector<DiscoveredMod> discover_mods(const std::filesystem::path& mods_dir, std::vector<std::string>& warnings);

struct SkippedMod {
    std::string id;
    std::string reason;
};

struct LoadPlan {
    std::vector<ModManifest> load_order;
    std::vector<SkippedMod> skipped;
    std::vector<std::string> warnings;
};

struct PlanInput {
    int loader_api_version = 1;
    std::string build_fingerprint;  // empty = unknown (couldn't hash GameAssembly)
    std::set<std::string> quarantined;
};

LoadPlan plan_load(const std::vector<DiscoveredMod>& mods, const LoaderConfig& cfg, const PlanInput& in);

}  // namespace lz
