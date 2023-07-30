#include "Exception.h"

namespace Application
{
namespace
{

constexpr int s_errorMessageBufferSize = 256;
std::array<char, s_errorMessageBufferSize> s_sourceCodeMessageBuffer = {};
std::array<char, s_errorMessageBufferSize * 2> s_errorMessageBuffer = {};

} // unnamed namespace

Exception::Exception(const char* message)
    : std::exception(message)
{
}

const char* FormatError(const char* sourceFile, int sourceLine, const char* functionName, HRESULT hr)
{
    return FormatError(sourceFile, sourceLine, functionName, "An error has occurred. HRESULT of 0x%08X.", hr);
}

const char* FormatError(const char* sourceFile, int sourceLine, const char* functionName, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf_s(&s_errorMessageBuffer[0], s_errorMessageBuffer.size(), format, args);
    va_end(args);

    sprintf_s(&s_sourceCodeMessageBuffer[0], s_sourceCodeMessageBuffer.size(), "\n%s %d\n%s\n", sourceFile, sourceLine, functionName);

    strcat_s(&s_errorMessageBuffer[0], s_errorMessageBuffer.size(), &s_sourceCodeMessageBuffer[0]);
    return &s_errorMessageBuffer[0];
}

} // namespace Application
