#include "core/config.h"
#include "core/ini.h"
#include "test_util.h"
#include "testing.h"

using namespace lz;

TEST_CASE("config: defaults when file missing") {
    auto c = load_config("definitely/not/here/lazarus.ini");
    CHECK(c.enabled);
    CHECK(!c.safe_mode);
    CHECK(c.enabled_mods.empty());  // missing config must not enable anything
    CHECK_EQ(c.warnings.size(), 1u);
}

TEST_CASE("config: shipped default text parses cleanly") {
    auto c = config_from_ini(Ini::parse(kDefaultConfigText));
    CHECK(c.warnings.empty());
    CHECK(c.enabled);
    CHECK_EQ(c.log_level, LogLevel::Info);
    CHECK_EQ(c.enabled_mods.size(), 1u);
}

TEST_CASE("config: values and enabled list") {
    TempDir td;
    write_file(td.path / "lazarus.ini",
               "[loader]\nsafe_mode=yes\nlog_level=trace\n[mods]\nenabled = B, a, b\n");
    auto c = load_config(td.path / "lazarus.ini");
    CHECK(c.safe_mode);
    CHECK_EQ(c.log_level, LogLevel::Trace);
    CHECK_EQ(c.enabled_mods.size(), 2u);
    if (c.enabled_mods.size() == 2) {
        CHECK_EQ(c.enabled_mods[0], "b");
        CHECK_EQ(c.enabled_mods[1], "a");
    }
}

TEST_CASE("config: invalid values fall back to defaults with warnings") {
    auto c = config_from_ini(Ini::parse("[loader]\nenabled=perhaps\nlog_level=shouty\nsafe_mod=true\n"));
    CHECK(c.enabled);
    CHECK_EQ(c.log_level, LogLevel::Info);
    CHECK_EQ(c.warnings.size(), 3u);
}
