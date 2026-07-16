#include "Core/Json.h"
#include <cstdio>
#include <cstdlib>

namespace ink {

namespace {

struct Parser {
    const std::string& s;
    std::size_t i = 0;
    std::string* err;

    void Fail(const char* msg) {
        if (err) *err = std::string("JSON error at offset ") + std::to_string(i) + ": " + msg;
        throw 1; // caught by Parse
    }
    void SkipWs() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r'))
            ++i;
    }
    bool Consume(char c) {
        SkipWs();
        if (i < s.size() && s[i] == c) { ++i; return true; }
        return false;
    }
    void Expect(char c, const char* what) {
        if (!Consume(c)) Fail(what);
    }

    Json ParseValue() {
        SkipWs();
        if (i >= s.size()) Fail("unexpected end of input");
        char c = s[i];
        if (c == '{') return ParseObject();
        if (c == '[') return ParseArray();
        if (c == '"') return Json(ParseString());
        if (c == 't') { Lit("true"); return Json(true); }
        if (c == 'f') { Lit("false"); return Json(false); }
        if (c == 'n') { Lit("null"); return Json(); }
        return ParseNumber();
    }

    void Lit(const char* word) {
        if (s.compare(i, std::string(word).size(), word) == 0) i += std::string(word).size();
        else Fail("invalid literal");
    }

    std::string ParseString() {
        Expect('"', "expected string");
        std::string out;
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"') return out;
            if (c == '\\') {
                if (i >= s.size()) Fail("bad escape");
                char e = s[i++];
                switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    if (i + 4 > s.size()) Fail("bad \\u escape");
                    unsigned cp = 0;
                    for (int k = 0; k < 4; ++k) {
                        char h = s[i++];
                        cp <<= 4;
                        if (h >= '0' && h <= '9') cp |= h - '0';
                        else if (h >= 'a' && h <= 'f') cp |= h - 'a' + 10;
                        else if (h >= 'A' && h <= 'F') cp |= h - 'A' + 10;
                        else Fail("bad hex in \\u");
                    }
                    // Encode BMP codepoint as UTF-8 (surrogates: encode the lone value).
                    if (cp < 0x80) out += static_cast<char>(cp);
                    else if (cp < 0x800) {
                        out += static_cast<char>(0xC0 | (cp >> 6));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        out += static_cast<char>(0xE0 | (cp >> 12));
                        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default: Fail("unknown escape");
                }
            } else {
                out += c;
            }
        }
        Fail("unterminated string");
        return out;
    }

    Json ParseNumber() {
        std::size_t start = i;
        if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
        bool any = false;
        while (i < s.size() && ((s[i] >= '0' && s[i] <= '9') || s[i] == '.' || s[i] == 'e' || s[i] == 'E' ||
                                s[i] == '+' || s[i] == '-')) {
            ++i;
            any = true;
        }
        if (!any) Fail("invalid number");
        const std::string num = s.substr(start, i - start);
        char* end = nullptr;
        double d = std::strtod(num.c_str(), &end);
        if (end == num.c_str()) Fail("invalid number");
        return Json(d);
    }

    Json ParseObject() {
        Expect('{', "expected object");
        Json obj = Json::Object();
        if (Consume('}')) return obj;
        for (;;) {
            std::string key = ParseString();
            Expect(':', "expected ':'");
            obj.Set(key, ParseValue());
            if (Consume(',')) continue;
            Expect('}', "expected '}'");
            break;
        }
        return obj;
    }

    Json ParseArray() {
        Expect('[', "expected array");
        Json arr = Json::Array();
        if (Consume(']')) return arr;
        for (;;) {
            arr.Push(ParseValue());
            if (Consume(',')) continue;
            Expect(']', "expected ']'");
            break;
        }
        return arr;
    }
};

void EscapeInto(const std::string& s, std::string& out) {
    out += '"';
    for (char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                out += buf;
            } else {
                out += c;
            }
        }
    }
    out += '"';
}

} // namespace

Json Json::Parse(const std::string& text, std::string* err) {
    try {
        Parser p{text, 0, err};
        Json v = p.ParseValue();
        p.SkipWs();
        if (p.i != text.size()) {
            if (err) *err = "trailing characters after JSON value";
            return Json();
        }
        return v;
    } catch (int) {
        return Json(); // err was filled by Parser::Fail
    }
}

void Json::SerializeInto(std::string& out, bool pretty, int depth) const {
    auto indent = [&](int d) {
        if (pretty) {
            out += '\n';
            out.append(static_cast<std::size_t>(d) * 2, ' ');
        }
    };
    switch (v.index()) {
    case 0: out += "null"; break;
    case 1: out += std::get<bool>(v) ? "true" : "false"; break;
    case 2: {
        double d = std::get<double>(v);
        if (d == static_cast<double>(static_cast<int64_t>(d)) && std::abs(d) < 9.0e15) {
            out += std::to_string(static_cast<int64_t>(d));
        } else {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.10g", d);
            out += buf;
        }
        break;
    }
    case 3: EscapeInto(std::get<std::string>(v), out); break;
    case 4: {
        const auto& obj = std::get<ObjectMap>(v);
        if (obj.empty()) { out += "{}"; break; }
        out += '{';
        bool first = true;
        for (const auto& [k, val] : obj) {
            if (!first) out += ',';
            first = false;
            indent(depth + 1);
            EscapeInto(k, out);
            out += pretty ? ": " : ":";
            val.SerializeInto(out, pretty, depth + 1);
        }
        indent(depth);
        out += '}';
        break;
    }
    case 5: {
        const auto& arr = std::get<ArrayVec>(v);
        if (arr.empty()) { out += "[]"; break; }
        out += '[';
        bool first = true;
        for (const auto& val : arr) {
            if (!first) out += ',';
            first = false;
            indent(depth + 1);
            val.SerializeInto(out, pretty, depth + 1);
        }
        indent(depth);
        out += ']';
        break;
    }
    }
}

std::string Json::Serialize(bool pretty) const {
    std::string out;
    SerializeInto(out, pretty, 0);
    return out;
}

} // namespace ink
