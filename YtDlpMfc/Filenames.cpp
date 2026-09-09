#include "pch.h"
#include "Filenames.h"

#include <algorithm>
#include <cwctype>
#include <set>

namespace {
const std::set<std::wstring> kReserved = {
    L"CON", L"PRN", L"AUX", L"NUL",
    L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
    L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9",
};
}

std::wstring SafeFilename(const std::wstring& name, size_t maxLen) {
    if (name.empty()) return L"video";

    std::wstring cleaned;
    cleaned.reserve(name.size());
    for (wchar_t c : name) {
        if (c < 0x20 || c == L'<' || c == L'>' || c == L':' || c == L'"' ||
            c == L'/' || c == L'\\' || c == L'|' || c == L'?' || c == L'*') {
            cleaned += L'_';
        } else {
            cleaned += c;
        }
    }

    // trim spaces and trailing dots
    while (!cleaned.empty() && (cleaned.front() == L' ' || cleaned.front() == L'\t'))
        cleaned.erase(cleaned.begin());
    while (!cleaned.empty() && (cleaned.back() == L' ' || cleaned.back() == L'.' || cleaned.back() == L'\t'))
        cleaned.pop_back();

    // collapse whitespace
    std::wstring collapsed;
    bool prevSpace = false;
    for (wchar_t c : cleaned) {
        if (c == L' ' || c == L'\t') {
            if (!prevSpace) collapsed += L' ';
            prevSpace = true;
        } else {
            collapsed += c;
            prevSpace = false;
        }
    }
    cleaned.swap(collapsed);

    if (cleaned.empty()) return L"video";
    if (cleaned.size() > maxLen) {
        cleaned.resize(maxLen);
        while (!cleaned.empty() && (cleaned.back() == L' ' || cleaned.back() == L'.'))
            cleaned.pop_back();
    }
    if (cleaned.empty()) return L"video";

    std::wstring base = cleaned;
    size_t dot = base.find(L'.');
    if (dot != std::wstring::npos) base = base.substr(0, dot);
    std::wstring upper = base;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::towupper);
    if (kReserved.count(upper)) {
        cleaned = L"_" + cleaned;
    }
    return cleaned;
}
