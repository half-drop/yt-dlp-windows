#include "pch.h"
#include "Config.h"

#include <fstream>
#include <sstream>
#include <map>

namespace {

std::string Narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring Widen(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

std::string JsonEscape(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '\\': o += "\\\\"; break;
        case '"': o += "\\\""; break;
        case '\n': o += "\\n"; break;
        case '\r': o += "\\r"; break;
        case '\t': o += "\\t"; break;
        default: o += c; break;
        }
    }
    return o;
}

// Extremely small key/value extractor for flat JSON objects.
bool ExtractString(const std::string& json, const char* key, std::string& out) {
    std::string pat = std::string("\"") + key + "\"";
    size_t p = json.find(pat);
    if (p == std::string::npos) return false;
    p = json.find(':', p + pat.size());
    if (p == std::string::npos) return false;
    p = json.find('"', p);
    if (p == std::string::npos) return false;
    size_t q = p + 1;
    std::string val;
    while (q < json.size()) {
        char c = json[q];
        if (c == '\\' && q + 1 < json.size()) {
            char n = json[q + 1];
            if (n == 'n') val += '\n';
            else if (n == 'r') val += '\r';
            else if (n == 't') val += '\t';
            else val += n;
            q += 2;
            continue;
        }
        if (c == '"') break;
        val += c;
        ++q;
    }
    out = val;
    return true;
}

bool ExtractBool(const std::string& json, const char* key, bool& out) {
    std::string pat = std::string("\"") + key + "\"";
    size_t p = json.find(pat);
    if (p == std::string::npos) return false;
    p = json.find(':', p + pat.size());
    if (p == std::string::npos) return false;
    while (p < json.size() && (json[p] == ':' || json[p] == ' ')) ++p;
    if (json.compare(p, 4, "true") == 0) { out = true; return true; }
    if (json.compare(p, 5, "false") == 0) { out = false; return true; }
    return false;
}

}  // namespace

std::wstring GetExeDir() {
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p(buf);
    size_t slash = p.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return p;
    return p.substr(0, slash);
}

std::wstring GetDefaultConfigPath() {
    return GetExeDir() + L"\\config.json";
}

AppConfig AppConfig::Load(const std::wstring& path) {
    AppConfig cfg;
    std::ifstream in(Narrow(path), std::ios::binary);
    if (!in) return cfg;
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string json = ss.str();

    std::string s;
    bool b = false;
    if (ExtractString(json, "output_dir", s)) cfg.outputDir = Widen(s);
    if (ExtractString(json, "browser", s) && !s.empty()) cfg.browser = Widen(s);
    if (ExtractString(json, "merge_format", s) && !s.empty()) cfg.mergeFormat = Widen(s);
    if (ExtractString(json, "last_url", s)) cfg.lastUrl = Widen(s);
    if (ExtractBool(json, "use_cookies", b)) cfg.useCookies = b;
    if (ExtractBool(json, "write_thumbnail", b)) cfg.writeThumbnail = b;
    return cfg;
}

bool AppConfig::Save(const std::wstring& path) const {
    std::ofstream out(Narrow(path), std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << "{\n"
        << "  \"output_dir\": \"" << JsonEscape(Narrow(outputDir)) << "\",\n"
        << "  \"use_cookies\": " << (useCookies ? "true" : "false") << ",\n"
        << "  \"browser\": \"" << JsonEscape(Narrow(browser)) << "\",\n"
        << "  \"write_thumbnail\": " << (writeThumbnail ? "true" : "false") << ",\n"
        << "  \"merge_format\": \"" << JsonEscape(Narrow(mergeFormat)) << "\",\n"
        << "  \"last_url\": \"" << JsonEscape(Narrow(lastUrl)) << "\"\n"
        << "}\n";
    return true;
}
