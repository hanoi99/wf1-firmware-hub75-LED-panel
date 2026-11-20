// Test-WebServer: ruft die vom WebController registrierten Routen wie ein Browser auf.
#pragma once
#include <Arduino.h>
#include <functional>
#include <map>
#include <string>
#include "LittleFS.h"
enum HTTPMethod { HTTP_GET = 1, HTTP_POST = 3 };
namespace wt {
inline std::map<std::string, std::function<void()>> routes;
inline std::function<void()> notFound;
inline std::map<std::string, std::string> args;
inline std::string curUri, outBody, outType, outEnc;
inline int outCode = 0;
}
class WebServer {
public:
    typedef std::function<void(void)> THandlerFunction;
    WebServer(int port = 80) {}
    void begin() {}
    void handleClient() {}
    void on(const String& uri, HTTPMethod m, THandlerFunction f) {
        wt::routes[std::string(m == HTTP_GET ? "GET " : "POST ") + uri.c_str()] = f;
    }
    void onNotFound(THandlerFunction f) { wt::notFound = f; }
    String arg(const String& n) const { auto it = wt::args.find(n.c_str()); return String(it == wt::args.end() ? "" : it->second.c_str()); }
    bool hasArg(const String& n) const { return wt::args.count(n.c_str()) > 0; }
    String uri() const { return String(wt::curUri.c_str()); }
    void sendHeader(const String& n, const String& v, bool first = false) {
        if (std::string(n.c_str()) == "Content-Encoding") wt::outEnc = v.c_str();
    }
    void send_P(int code, const char* type, const char* content, size_t len) { wt::outCode = code; wt::outType = type; wt::outBody = "<binary>"; }
    void send(int code, const char* type, const String& content) { wt::outCode = code; wt::outType = type; wt::outBody = content.c_str(); }
    void send(int code, const char* type, const char* content) { wt::outCode = code; wt::outType = type; wt::outBody = content; }
    void send(int code, const String& type, const String& content) { wt::outCode = code; wt::outType = type.c_str(); wt::outBody = content.c_str(); }
    size_t streamFile(File& f, const String& contentType) { return 0; }
    bool authenticate(const char* user, const char* pass) { return true; }
    void requestAuthentication() {}
};
