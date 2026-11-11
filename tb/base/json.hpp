// json.hpp -- tiny streaming JSON writer (no external dependency).
#pragma once
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

inline std::string json_escape(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    o += buf;
                } else {
                    o += c;
                }
        }
    }
    return o;
}

// Usage: JsonWriter w; w.begin_obj(); w.key("a").val(1); w.end_obj(); w.str();
class JsonWriter {
public:
    JsonWriter& begin_obj() { pre(); os_ << "{"; first_.push_back(true); return *this; }
    JsonWriter& end_obj() { os_ << "}"; first_.pop_back(); return *this; }
    JsonWriter& begin_arr() { pre(); os_ << "["; first_.push_back(true); return *this; }
    JsonWriter& end_arr() { os_ << "]"; first_.pop_back(); return *this; }
    JsonWriter& key(const std::string& k) {
        pre();
        os_ << "\"" << json_escape(k) << "\":";
        after_key_ = true;
        return *this;
    }
    JsonWriter& val(const std::string& s) { pre(); os_ << "\"" << json_escape(s) << "\""; return *this; }
    JsonWriter& val(const char* s) { return val(std::string(s)); }
    JsonWriter& val(bool b) { pre(); os_ << (b ? "true" : "false"); return *this; }
    JsonWriter& val(double d) {
        pre();
        char buf[32];
        std::snprintf(buf, sizeof buf, "%.6g", d);
        os_ << buf;
        return *this;
    }
    template <class I, class = std::enable_if_t<std::is_integral<I>::value && !std::is_same<I, bool>::value>>
    JsonWriter& val(I i) { pre(); os_ << static_cast<long long>(i); return *this; }
    JsonWriter& raw(const std::string& r) { pre(); os_ << r; return *this; }

    std::string str() const { return os_.str(); }

private:
    void pre() {
        if (after_key_) { after_key_ = false; return; }
        if (!first_.empty()) {
            if (!first_.back()) os_ << ",";
            first_.back() = false;
        }
    }
    std::ostringstream os_;
    std::vector<bool> first_;
    bool after_key_ = false;
};
