#pragma once

#include <Windows.h>

#include <stdexcept>

namespace Application
{

class Exception : public std::exception
{
public:
    explicit Exception(const char* message);
};

const char* FormatError(HRESULT hr, const char* sourceFile, int sourceLine, const char* functionName);

} // namespace Application

#define GM_FORMAT_ERROR(hr) Application::FormatError(hr, __FILE__, __LINE__, __FUNCSIG__)
