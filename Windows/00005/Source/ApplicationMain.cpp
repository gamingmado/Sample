#include "ApplicationMain.h"

#include "Application.h"

namespace Application
{

int Main(HINSTANCE hInstance, int nCmdShow)
{
    int returnCode = EXIT_FAILURE;
    try
    {
        Application application(hInstance, nCmdShow);
        returnCode = application.Run();
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
