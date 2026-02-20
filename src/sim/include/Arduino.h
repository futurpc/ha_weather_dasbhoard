// Mock Arduino.h for native simulator builds
#pragma once

#include <string>
#include <cstdint>
#include <cstddef>
#include <cstring>

// Minimal Arduino String class shim
class String {
    std::string _s;
public:
    String() {}
    String(const char* s) : _s(s ? s : "") {}
    String(const std::string& s) : _s(s) {}
    const char* c_str() const { return _s.c_str(); }
    size_t length() const { return _s.length(); }
    bool operator==(const char* o) const { return _s == o; }
    bool operator==(const String& o) const { return _s == o._s; }
    String& operator=(const char* s) { _s = s ? s : ""; return *this; }
};
