#include <windows.h>

int WINAPI wWinMain(
    _In_ HINSTANCE,
    _In_opt_ HINSTANCE,
    _In_ LPWSTR,
    _In_ int)
{
    MessageBox(nullptr, L"Hello World!", L"Title", MB_OK);
    return 0;
}
