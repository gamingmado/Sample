#include "FormatMessage.h"

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    int returnCode = EXIT_FAILURE;
    try
    {
        bool isSuccess = true;

        // ここで何かの処理を行う
        // ...
        // 途中で失敗した場合に例外を投げる

        if (isSuccess)
        {
            throw std::runtime_error(FORMAT_MESSAGE(E_FAIL));
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
