#include "ApplicationMain.h"

#include "Exception.h"

namespace Application
{

int Main([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] int nCmdShow)
{
    int returnCode = EXIT_FAILURE;
    try
    {
        bool isSuccess = true;
        if (!isSuccess)
        {
            throw APPLICATION_HRESULT_EXCEPTION(E_FAIL);
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

} // namespace Application
