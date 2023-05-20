#pragma once

std::string Format(HRESULT hr, const char* sourceFile, int sourceLine, const char* functionName);

#define FORMAT_MESSAGE(hr) \
    Format(hr, __FILE__, __LINE__, __FUNCSIG__)
