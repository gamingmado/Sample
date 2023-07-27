#include "Application.h"

namespace Application
{

Body::Body(HINSTANCE hInstance, int nCmdShow)
    : m_window(hInstance)
    , m_graphics(m_window)
{
    ShowWindow(m_window.GetHandle(), nCmdShow);
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
        else
        {
            m_graphics.Render();
        }
    }
    return static_cast<int>(msg.wParam);
}

} // namespace Application
