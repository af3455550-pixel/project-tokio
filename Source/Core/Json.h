#pragma once
// Minimal dependency-free JSON: parse + serialize + typed access.
// Data-driven design (§71): weapons, charms, enemies, bosses, quests,
// achievements, worlds and settings are all stored as JSON.
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace ink {

class Json {
public:
    using Null = std::monostate;
    using ObjectMap = std::map<std::string, Json>;
    using ArrayVec = std::vector<Json>;

    Json() = default;
    Json(bool b) : v(b) {}
    Json(double d) : v(d) {}
    Json(int i) : v(static_cast<double>(i)) {}
    Json(const char* s) : v(std::string(s)) {}
    Json(std::string s) : v(std::move(s)) {}

    static Json Object() { Json j; j.v = ObjectMap{}; return j; }
    static Json Array() { Json j; j.v = ArrayVec{}; return j; }

    bool IsNull() const { return std::holds_alternative<Null>(v); }
    bool IsBool() const { return std::holds_alternative<bool>(v); }
    bool IsNumber() const { return std::holds_alternative<double>(v); }
    bool IsString() const { return std::holds_alternative<std::string>(v); }
    bool IsObject() const { return std::holds_alternative<ObjectMap>(v); }
    bool IsArray() const { return std::holds_alternative<ArrayVec>(v); }

    double AsNumber(double def = 0.0) const { return IsNumber() ? std::get<double>(v) : def; }
    int AsInt(int def = 0) const { return IsNumber() ? static_cast<int>(std::get<double>(v)) : def; }
    bool AsBool(bool def = false) const { return IsBool() ? std::get<bool>(v) : def; }
    std::string AsString(const std::string& def = "") const { return IsString() ? std::get<std::string>(v) : def; }

    // Object access (never crashes on wrong shape: returns nullptr).
    const Json* Find(const std::string& key) const {
        if (!IsObject()) return nullptr;
        auto it = std::get<ObjectMap>(v).find(key);
        return it == std::get<ObjectMap>(v).end() ? nullptr : &it->second;
    }
    Json& Set(const std::string& key, Json value) {
        if (!IsObject()) v = ObjectMap{};
        std::get<ObjectMap>(v)[key] = std::move(value);
        return std::get<ObjectMap>(v)[key];
    }

    // Array access.
    std::size_t Size() const {
        if (IsArray()) return std::get<ArrayVec>(v).size();
        if (IsObject()) return std::get<ObjectMap>(v).size();
        return 0;
    }
    const Json& At(std::size_t i) const {
        static const Json kNull;
        if (!IsArray() || i >= std::get<ArrayVec>(v).size()) return kNull;
        return std::get<ArrayVec>(v)[i];
    }
    ArrayVec& Push(Json value) {
        if (!IsArray()) v = ArrayVec{};
        std::get<ArrayVec>(v).push_back(std::move(value));
        return std::get<ArrayVec>(v);
    }
    const ArrayVec& Items() const {
        static const ArrayVec kEmpty;
        return IsArray() ? std::get<ArrayVec>(v) : kEmpty;
    }
    const ObjectMap& Members() const {
        static const ObjectMap kEmpty;
        return IsObject() ? std::get<ObjectMap>(v) : kEmpty;
    }

    // Parsing. On failure returns a Null Json and (optionally) an error message.
    static Json Parse(const std::string& text, std::string* err = nullptr);

    // Serialization. pretty=true -> 2-space indent.
    std::string Serialize(bool pretty = true) const;

private:
    void SerializeInto(std::string& out, bool pretty, int depth) const;
    std::variant<Null, bool, double, std::string, ObjectMap, ArrayVec> v;
};

} // namespace ink
