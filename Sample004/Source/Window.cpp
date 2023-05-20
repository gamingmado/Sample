#include "Window.h"

#include "FormatMessage.h"

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

Window::Window(HINSTANCE hInstance, int nCmdShow)
{
    Initialize(hInstance, nCmdShow);
}

Window::~Window()
{
}

int Window::Run()
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

void Window::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEX windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = L"WindowClass";
    if (RegisterClassEx(&windowClass) == 0)
    {
        throw std::runtime_error(FORMAT_MESSAGE("RegisterClassEx failed.\n"));
    }

    RECT rect = { 0, 0, 1280, 760 };
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rect, style, FALSE);
    HWND hWnd = CreateWindowEx(
        0,
        windowClass.lpszClassName,
        L"Title",
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
        throw std::runtime_error(FORMAT_MESSAGE("CreateWindowEx failed.\n"));
    }

    ShowWindow(hWnd, nCmdShow);
}
