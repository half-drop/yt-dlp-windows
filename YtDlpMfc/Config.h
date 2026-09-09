#pragma once

#include <string>

// JSON-ish lightweight config (no external deps).
struct AppConfig {
    std::wstring outputDir;
    bool useCookies = false;
    std::wstring browser = L"chrome";
    bool writeThumbnail = true;
    std::wstring mergeFormat = L"mp4";
    std::wstring lastUrl;

    static AppConfig Load(const std::wstring& path);
    bool Save(const std::wstring& path) const;
};

std::wstring GetDefaultConfigPath();
std::wstring GetExeDir();
