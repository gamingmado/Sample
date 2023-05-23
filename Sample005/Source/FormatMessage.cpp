#include "FormatMessage.h"

std::string Format(const char* sourceFile, int sourceLine, const char* functionName, const char* format, ...)
{
    char str[256] = {};
    va_list args;
    va_start(args, format);
    vsprintf_s(str, (sizeof(str) / sizeof(str[0])), format, args);
    va_end(args);

    char str2[256] = {};
    sprintf_s(str2, (sizeof(str2) / sizeof(str2[0])), "%s %d\n%s\n", sourceFile, sourceLine, functionName);

    std::string s(str);
    s += str2;

    return s;
}

std::string Format(HRESULT hr, const char* sourceFile, int sourceLine, const char* functionName)
{
    char str[256] = {};
    sprintf_s(str, "An error has occurred. HRESULT of 0x%08X.\n%s %d\n%s\n", static_cast<UINT>(hr), sourceFile, sourceLine, functionName);
    return std::string(str);
}
