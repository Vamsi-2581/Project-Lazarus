#include "core/config.h"

#include <set>

#include "core/ini.h"

namespace lz {

const char* const kDefaultConfigText =
    "; Project Lazarus loader settings\r\n"
    "; Lines starting with ; are comments.\r\n"
    "\r\n"
    "[loader]\r\n"
    "; false = Lazarus stays completely inactive (the game runs as if it were not installed)\r\n"
    "enabled = true\r\n"
    "; true = start the loader but load no mods. Holding SHIFT while the game starts does the same.\r\n"
    "safe_mode = false\r\n"
    "; trace, debug, info, warn, error\r\n"
    "log_level = info\r\n"
    "; mods can declare which game builds they were tested on. true = load them anyway on other builds.\r\n"
    "ignore_build_check = false\r\n"
    "\r\n"
    "[mods]\r\n"
    "; Mods are native code with full access to your PC. Only enable mods you trust.\r\n"
    "; Comma-separated mod ids, loaded in this order (dependencies are loaded first automatically).\r\n"
    "enabled = graphics-settings\r\n";

static void read_bool(const Ini& ini, const char* key, bool& field, LoaderConfig& c) {
    auto v = ini.get("loader", key);
    if (!v) return;
    if (!parse_bool(*v, field))
        c.warnings.push_back(std::string("[loader] ") + key + " = '" + *v + "' is not true/false, using default");
}

LoaderConfig config_from_ini(const Ini& ini) {
    LoaderConfig c;
    for (auto& e : ini.errors())
        c.warnings.push_back("lazarus.ini line " + std::to_string(e.line) + ": " + e.message);

    read_bool(ini, "enabled", c.enabled, c);
    read_bool(ini, "safe_mode", c.safe_mode, c);
    read_bool(ini, "ignore_build_check", c.ignore_build_check, c);

    if (auto v = ini.get("loader", "log_level")) {
        if (!parse_level(*v, c.log_level))
            c.warnings.push_back("[loader] log_level = '" + *v + "' unknown, using info");
    }

    std::set<std::string> seen;
    for (auto& id : split_list(ini.get_or("mods", "enabled", ""))) {
        auto lid = to_lower(id);
        if (seen.insert(lid).second) c.enabled_mods.push_back(lid);
    }

    static const std::set<std::string> known_loader = {"enabled", "safe_mode", "log_level", "ignore_build_check"};
    for (auto& k : ini.keys("loader"))
        if (!known_loader.count(k)) c.warnings.push_back("unknown key '" + k + "' in [loader]");
    return c;
}

LoaderConfig load_config(const std::filesystem::path& path) {
    Ini ini;
    std::string err;
    if (!Ini::load(path, ini, err)) {
        LoaderConfig c;
        c.warnings.push_back("no config at " + path.u8string() + ", using defaults (no mods enabled)");
        return c;
    }
    return config_from_ini(ini);
}

}  // namespace lz
