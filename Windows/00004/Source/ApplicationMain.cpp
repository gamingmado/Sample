#include "ApplicationMain.h"

namespace Application
{
namespace
{

constexpr wchar_t s_windowClassName[] = L"WindowClass";
constexpr wchar_t s_windowTitleName[] = L"Title";
constexpr LONG s_windowWidth = 1280;
constexpr LONG s_windowHeight = 760;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

} // unused namespace

int Main([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] int nCmdShow)
{
    int returnCode = EXIT_FAILURE;
    try
    {
        WNDCLASSEX windowClass = {};
        windowClass.cbSize = sizeof(WNDCLASSEX);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = hInstance;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.lpszClassName = s_windowClassName;
        if (RegisterClassEx(&windowClass) == 0)
        {
            // throw Application::Exception();
        }

        RECT rect = { 0, 0, s_windowWidth, s_windowHeight };
        DWORD style = WS_OVERLAPPEDWINDOW;
        AdjustWindowRect(&rect, style, FALSE);
        HWND hWnd = CreateWindowEx(
            0,
            windowClass.lpszClassName,
            s_windowTitleName,
            style,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            hInstance,
            nullptr);
        if (hWnd == nullptr)
        {
            // throw Application::Exception();
        }

        ShowWindow(hWnd, nCmdShow);

        MSG msg = {};
        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        returnCode = static_cast<int>(msg.wParam);
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

} // namespace Application
