#include "Application.h"

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

Body::Body(HINSTANCE hInstance, int nCmdShow)
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
        throw APPLICATION_EXCEPTION("RegisterClassEx failed.");
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
        throw APPLICATION_EXCEPTION("CreateWindowEx failed.");
    }

    ShowWindow(hWnd, nCmdShow);
}

int Body::Run()
{
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return static_cast<int>(msg.wParam);
}

} // namespace Application
