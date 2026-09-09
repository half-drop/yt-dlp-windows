#pragma once

#include <string>

// Sanitize a string for use as a Windows filename stem.
std::wstring SafeFilename(const std::wstring& name, size_t maxLen = 180);
