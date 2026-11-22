#pragma once
// Minimale Arduino-Umgebung für den Host-Simulator (nur was Szenen/Manager brauchen).
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cctype>
#include <ctime>
#include "Print.h"
#include "pgmspace.h"
#ifndef ARDUINO
#define ARDUINO 200
#endif
using std::abs; using std::max; using std::min;
extern "C++" {
template <class T, class L> auto min(const T& a, const L& b) -> decltype((b < a) ? b : a) { return (b < a) ? b : a; }
template <class T, class L> auto max(const T& a, const L& b) -> decltype((b < a) ? b : a) { return (a < b) ? b : a; }
}
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define PI 3.1415926535897932384626433832795
#define radians(deg) ((deg) * 0.017453292519943295)
#define degrees(rad) ((rad) * 57.29577951308232)
inline bool isDigit(int c) { return c >= '0' && c <= '9'; }

// simulierte Uhr
extern unsigned long sim_ms;
inline unsigned long millis() { return sim_ms; }
inline unsigned long micros() { return sim_ms * 1000UL; }
inline void delay(unsigned long) {}
inline long random(long a, long b) { return a + (std::rand() % (b - a)); }
inline long random(long a) { return std::rand() % a; }
inline void randomSeed(unsigned long s) { std::srand(s); }

class String {
public:
    String() {}
    String(const char* s) : s_(s ? s : "") {}
    String(const std::string& s) : s_(s) {}
    String(char c) : s_(1, c) {}
    String(unsigned char v) : s_(std::to_string(v)) {}
    String(int v) : s_(std::to_string(v)) {}
    String(unsigned v) : s_(std::to_string(v)) {}
    String(long v) : s_(std::to_string(v)) {}
    String(unsigned long v) : s_(std::to_string(v)) {}
    String(float v, int d = 2) { char b[32]; snprintf(b, sizeof b, "%.*f", d, v); s_ = b; }
    size_t length() const { return s_.size(); }
    const char* c_str() const { return s_.c_str(); }
    char operator[](size_t i) const { return i < s_.size() ? s_[i] : 0; }
    char& operator[](size_t i) { return s_[i]; }
    char charAt(size_t i) const { return (*this)[i]; }
    void reserve(size_t n) { s_.reserve(n); }
    String& operator+=(const String& o) { s_ += o.s_; return *this; }
    String& operator+=(const char* o) { s_ += o; return *this; }
    String& operator+=(char c) { s_ += c; return *this; }
    bool concat(const String& o) { s_ += o.s_; return true; }
    friend String operator+(const String& a, const String& b) { return String(a.s_ + b.s_); }
    friend String operator+(const String& a, const char* b) { return String(a.s_ + b); }
    friend String operator+(const char* a, const String& b) { return String(std::string(a) + b.s_); }
    bool operator==(const String& o) const { return s_ == o.s_; }
    bool operator==(const char* o) const { return s_ == o; }
    bool operator!=(const String& o) const { return s_ != o.s_; }
    bool operator!=(const char* o) const { return s_ != o; }
    String substring(size_t from, size_t to = (size_t)-1) const {
        if (from > s_.size()) return String();
        return String(s_.substr(from, to == (size_t)-1 ? std::string::npos : (to > from ? to - from : 0)));
    }
    int indexOf(const String& o, size_t from = 0) const { auto p = s_.find(o.s_, from); return p == std::string::npos ? -1 : (int)p; }
    int indexOf(char c, size_t from = 0) const { auto p = s_.find(c, from); return p == std::string::npos ? -1 : (int)p; }
    bool startsWith(const String& o) const { return s_.rfind(o.s_, 0) == 0; }
    bool endsWith(const String& o) const { return s_.size() >= o.s_.size() && s_.compare(s_.size() - o.s_.size(), o.s_.size(), o.s_) == 0; }
    long toInt() const { return std::strtol(s_.c_str(), nullptr, 10); }
    void trim() { size_t a = s_.find_first_not_of(" \t\r\n"); size_t b = s_.find_last_not_of(" \t\r\n"); s_ = a == std::string::npos ? "" : s_.substr(a, b - a + 1); }
    void remove(size_t idx, size_t count = (size_t)-1) { if (idx < s_.size()) s_.erase(idx, count); }
    void replace(char a, char b) { for (auto& c : s_) if (c == a) c = b; }
    void replace(const String& a, const String& b) { size_t p = 0; while ((p = s_.find(a.s_, p)) != std::string::npos) { s_.replace(p, a.s_.size(), b.s_); p += b.s_.size(); } }
    bool isEmpty() const { return s_.empty(); }
private:
    std::string s_;
};

class SerialClass : public Print {
public:
    void begin(unsigned long) {}
    size_t write(uint8_t c) override { fputc(c, stderr); return 1; }
    int printf(const char* f, ...) { va_list a; va_start(a, f); int n = vfprintf(stderr, f, a); va_end(a); return n; }
    void println(const char* s = "") { fprintf(stderr, "%s\n", s); }
    void println(const String& s) { fprintf(stderr, "%s\n", s.c_str()); }
    void print(const char* s) { fprintf(stderr, "%s", s); }
};
extern SerialClass Serial;
struct EspClass { unsigned getFreeHeap() { return 123456; } unsigned getMinFreeHeap() { return 100000; } unsigned getMaxAllocHeap() { return 90000; } void restart() {} };
extern EspClass ESP;
