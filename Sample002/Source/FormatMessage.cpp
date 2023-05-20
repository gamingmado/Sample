#include "FormatMessage.h"

std::string Format(HRESULT hr, const char* sourceFile, int sourceLine, const char* functionName)
{
    char str[256] = {};
    sprintf_s(str, "An error has occurred. HRESULT of 0x%08X.\n%s %d\n%s\n", static_cast<UINT>(hr), sourceFile, sourceLine, functionName);
    return std::string(str);
}
