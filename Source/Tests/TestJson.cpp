#include "Core/Json.h"
#include "Test.h"

using namespace ink;

INK_TEST(json_scalars) {
    Json j = Json::Parse("42.5");
    INK_REQUIRE(j.IsNumber());
    INK_REQUIRE_NEAR(j.AsNumber(), 42.5, 1e-12);
    INK_REQUIRE(Json::Parse("true").AsBool());
    INK_REQUIRE(!Json::Parse("false").AsBool());
    INK_REQUIRE(Json::Parse("null").IsNull());
}

INK_TEST(json_string_and_escapes) {
    Json j = Json::Parse("\"a\\\"b\\\\c\\n\"");
    INK_REQUIRE(j.IsString());
    INK_REQUIRE(j.AsString() == "a\"b\\c\n");
    // \u escape must come back as UTF-8 (é = U+00E9 -> C3 A9). "Aurlio" = 6
    // chars, but é is 2 bytes: 8 bytes total, é at byte offsets 3..4.
    Json e = Json::Parse("\"Aur\\u00e9lio\"");
    const std::string& s = e.AsString();
    INK_REQUIRE_EQ(s.size(), static_cast<std::size_t>(8));
    INK_REQUIRE(s[3] == '\xC3' && s[4] == '\xA9');
    INK_REQUIRE(s.compare(0, 1, "A") == 0);
}

INK_TEST(json_nested_access) {
    Json j = Json::Parse(R"({"a": {"b": [1, 2, 3]}, "c": "x"})");
    const Json* b = j.Find("a")->Find("b");
    INK_REQUIRE(b->IsArray());
    INK_REQUIRE_EQ(static_cast<int>(b->Size()), 3);
    INK_REQUIRE_NEAR(b->At(2).AsNumber(), 3.0, 1e-12);
    INK_REQUIRE(j.Find("c")->AsString() == "x");
    INK_REQUIRE(j.Find("missing") == nullptr);
    INK_REQUIRE(j.Find("a")->Find("nope") == nullptr);
}

INK_TEST(json_errors) {
    std::string err;
    INK_REQUIRE(Json::Parse("{oops", &err).IsNull());
    INK_REQUIRE(!err.empty());
    INK_REQUIRE(Json::Parse("[1, 2", &err).IsNull());
    INK_REQUIRE(Json::Parse("\"unterminated", &err).IsNull());
    INK_REQUIRE(Json::Parse("{} trailing", &err).IsNull());
    // A parse error must not crash and must leave a message.
    err.clear();
    INK_REQUIRE(Json::Parse("\"\\uZZZZ\"", &err).IsNull());
    INK_REQUIRE(!err.empty());
}

INK_TEST(json_roundtrip) {
    Json j = Json::Object();
    j.Set("name", Json("Milo"));
    j.Set("hp", Json(3));
    j.Set("hp2", Json(3.0));
    j.Set("ok", Json(true));
    Json arr = Json::Array();
    arr.Push(Json("a"));
    arr.Push(Json("b"));
    j.Set("tags", std::move(arr));
    const std::string text = j.Serialize(true);
    Json k = Json::Parse(text);
    INK_REQUIRE(!k.IsNull());
    INK_REQUIRE(k.Find("name")->AsString() == "Milo");
    INK_REQUIRE_EQ(k.Find("hp")->AsInt(), 3);
    INK_REQUIRE_EQ(k.Find("ok")->AsBool(), true);
    INK_REQUIRE_EQ(static_cast<int>(k.Find("tags")->Size()), 2);
}
