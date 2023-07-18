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

const char* FormatError(const char* sourceFile, int sourceLine, const char* functionName, const char* format, ...);
const char* FormatError(const char* sourceFile, int sourceLine, const char* functionName, HRESULT hr);

} // namespace Application

#define APPLICATION_EXCEPTION(format, ...) Application::Exception(Application::FormatError(__FILE__, __LINE__, __FUNCSIG__, format, __VA_ARGS__))
#define APPLICATION_HRESULT_EXCEPTION(hr) Application::Exception(Application::FormatError(__FILE__, __LINE__, __FUNCSIG__, hr))
