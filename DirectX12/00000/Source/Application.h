#pragma once

#include "DX12Graphics.h"
#include "Window.h"

namespace Application
{

class Body
{
public:
    explicit Body(HINSTANCE hInstance, int nCmdShow);

    int Run();

private:
    Window m_window;
    DX12Graphics m_graphics;
};

} // namespace Application
