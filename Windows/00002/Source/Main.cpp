#include <Windows.h>

#include "Exception.h"

int WINAPI wWinMain(
    _In_ HINSTANCE,
    _In_opt_ HINSTANCE,
    _In_ LPWSTR,
    _In_ int)
{
    int returnCode = EXIT_FAILURE;
    try
    {
        bool isSuccess = true;
        if (!isSuccess)
        {
            throw Application::Exception(GM_FORMAT_ERROR(E_FAIL));
        }
        returnCode = EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA(e.what());
    }
    catch (...)
    {
        OutputDebugStringA("An unknown error has occurred.\n");
    }

    return returnCode;
}
