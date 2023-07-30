#pragma once

namespace Application
{

class Window
{
public:
    explicit Window(HINSTANCE hInstance);

    HWND GetHandle() { return m_handle; }
    LONG GetWidth() const { return m_width; }
    LONG GetHeight() const { return m_height; }

private:
    HWND m_handle;
    LONG m_width;
    LONG m_height;
};

} // namespace Application
