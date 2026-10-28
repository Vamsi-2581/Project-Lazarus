#include "core/ini.h"
#include "testing.h"

using namespace lz;

TEST_CASE("ini: basic sections and keys") {
    auto ini = Ini::parse("[Loader]\nEnabled = true\nlog_level=debug\n\n[mods]\nenabled = a, b\n");
    CHECK(ini.errors().empty());
    CHECK_EQ(ini.get_or("loader", "enabled", ""), "true");
    CHECK_EQ(ini.get_or("LOADER", "LOG_LEVEL", ""), "debug");
    CHECK_EQ(ini.get_or("mods", "enabled", ""), "a, b");
    CHECK(!ini.get("mods", "missing").has_value());
    CHECK(!ini.get("nosuch", "enabled").has_value());
}

TEST_CASE("ini: comments, blank lines, BOM, CRLF") {
    auto ini = Ini::parse("\xEF\xBB\xBF; header comment\r\n# another\r\n[a]\r\nx = 1 ; trailing\r\n\r\ny=2\r\n");
    CHECK(ini.errors().empty());
    CHECK_EQ(ini.get_or("a", "x", ""), "1");
    CHECK_EQ(ini.get_or("a", "y", ""), "2");
}

TEST_CASE("ini: semicolon inside value without leading space is kept") {
    auto ini = Ini::parse("[p]\npath=C:\\games;stuff\n");
    CHECK_EQ(ini.get_or("p", "path", ""), "C:\\games;stuff");
}

TEST_CASE("ini: malformed lines are reported with line numbers, not fatal") {
    auto ini = Ini::parse("[ok]\na=1\n[broken\nnot a pair\n=novalue\na=2\n");
    CHECK_EQ(ini.errors().size(), 4u);
    if (ini.errors().size() == 4) {
        CHECK_EQ(ini.errors()[0].line, 3);
        CHECK_EQ(ini.errors()[1].line, 4);
        CHECK_EQ(ini.errors()[2].line, 5);
        CHECK_EQ(ini.errors()[3].line, 6);  // duplicate key warning
    }
    CHECK_EQ(ini.get_or("ok", "a", ""), "2");
}

TEST_CASE("ini: empty section still exists") {
    auto ini = Ini::parse("[empty]\n[full]\nk=v\n");
    CHECK(ini.has_section("empty"));
    CHECK(ini.keys("empty").empty());
    CHECK_EQ(ini.keys("full").size(), 1u);
}

TEST_CASE("ini: load missing file fails cleanly") {
    Ini ini;
    std::string err;
    CHECK(!Ini::load("this/file/does/not/exist.ini", ini, err));
    CHECK(!err.empty());
}

TEST_CASE("ini: value helpers") {
    bool b = false;
    CHECK(parse_bool(" Yes ", b) && b);
    CHECK(parse_bool("off", b) && !b);
    CHECK(!parse_bool("maybe", b));

    long long i = 0;
    CHECK(parse_int("-42", i) && i == -42);
    CHECK(!parse_int("12abc", i));
    CHECK(!parse_int("", i));
    CHECK(!parse_int("99999999999999999999999", i));

    double d = 0;
    CHECK(parse_float("1.5", d) && d == 1.5);
    CHECK(!parse_float("1.5x", d));

    auto l = split_list(" a, ,b,, c ");
    CHECK_EQ(l.size(), 3u);
    CHECK(split_list("").empty());
}
