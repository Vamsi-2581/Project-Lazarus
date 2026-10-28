#include "../mods/graphics-settings/settings.h"
#include "core/ini.h"
#include "testing.h"

using namespace gfx;

static Settings parse_text(const char* text, std::vector<std::string>& w) {
    auto ini = lz::Ini::parse(text);
    return parse([&](const char* s, const char* k) { return ini.get(s, k); }, w);
}

TEST_CASE("gfx: shipped default config changes nothing") {
    std::vector<std::string> w;
    auto s = parse_text(kDefaultConfig, w);
    CHECK(w.empty());
    CHECK_EQ(s.window_mode, (int)kLeave);
    CHECK_EQ(s.width, 0);
    CHECK_EQ(s.target_fps, 0);
    CHECK_EQ(s.vsync, -1);
    CHECK_EQ(s.quality_level, -1);
    CHECK(s.lod_bias == 0);
    CHECK_EQ(s.anisotropic, -1);
    CHECK(s.reapply_each_scene);
    CHECK(!s.touches_display());
}

TEST_CASE("gfx: full valid config") {
    std::vector<std::string> w;
    auto s = parse_text("[display]\nwindow_mode = Borderless\nwidth=2560\nheight=1440\nrefresh_rate=144\n"
                        "[performance]\ntarget_fps=-1\nvsync=0\n[quality]\nquality_level=2\nlod_bias=1.5\n"
                        "anisotropic=force\n[advanced]\nreapply_each_scene=false\n",
                        w);
    CHECK(w.empty());
    CHECK_EQ(s.window_mode, (int)kBorderless);
    CHECK_EQ(s.width, 2560);
    CHECK_EQ(s.height, 1440);
    CHECK_EQ(s.refresh_rate, 144);
    CHECK_EQ(s.target_fps, -1);
    CHECK_EQ(s.vsync, 0);
    CHECK_EQ(s.quality_level, 2);
    CHECK(s.lod_bias == 1.5);
    CHECK_EQ(s.anisotropic, 2);
    CHECK(!s.reapply_each_scene);
    CHECK(s.touches_display());
}

TEST_CASE("gfx: bad values are ignored with warnings, never fatal") {
    std::vector<std::string> w;
    auto s = parse_text("[display]\nwindow_mode=huge\nwidth=1920\n[performance]\ntarget_fps=5\nvsync=9\n"
                        "[quality]\nlod_bias=-2\nanisotropic=max\nquality_level=abc\n[advanced]\nreapply_each_scene=sometimes\n",
                        w);
    CHECK_EQ(s.window_mode, (int)kLeave);
    CHECK_EQ(s.width, 0);  // height missing -> resolution dropped
    CHECK_EQ(s.target_fps, 0);
    CHECK_EQ(s.vsync, -1);
    CHECK(s.lod_bias == 0);
    CHECK_EQ(s.anisotropic, -1);
    CHECK_EQ(s.quality_level, -1);
    CHECK(s.reapply_each_scene);
    CHECK_EQ(w.size(), 8u);
}

TEST_CASE("gfx: vsync + fps cap conflict warns") {
    std::vector<std::string> w;
    auto s = parse_text("[performance]\ntarget_fps=60\nvsync=1\n", w);
    CHECK_EQ(s.target_fps, 60);
    CHECK_EQ(s.vsync, 1);
    CHECK_EQ(w.size(), 1u);
}
