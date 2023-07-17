#include "Exception.h"

namespace Application
{
namespace
{

char s_errorMessageBuffer[256] = {};

} // unnamed namespace

Exception::Exception(const char* message)
    : std::exception(message)
{
}

const char* FormatError(HRESULT hr, const char* sourceFile, int sourceLine, const char* functionName)
{
    sprintf_s(s_errorMessageBuffer, "An error has occurred.\nHRESULT of 0x%08X.\n%s %d\n%s\n", static_cast<UINT>(hr), sourceFile, sourceLine, functionName);
    return s_errorMessageBuffer;
}

} // namespace Application
