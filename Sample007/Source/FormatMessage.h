#pragma once

std::string Format(const char* sourceFile, int sourceLine, const char* functionName, const char* format, ...);
std::string Format(HRESULT hr, const char* sourceFile, int sourceLine, const char* functionName);

#define FORMAT_MESSAGE(format, ...) \
    Format(__FILE__, __LINE__, __FUNCSIG__, format, __VA_ARGS__)

#define FORMAT_HRESULT_MESSAGE(hr) \
    Format(hr, __FILE__, __LINE__, __FUNCSIG__)
