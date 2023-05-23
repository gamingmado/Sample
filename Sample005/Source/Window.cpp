#include "Window.h"

#include "FormatMessage.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

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
        else
        {
            Render();
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

    InitializeGraphics(hWnd);

    ShowWindow(hWnd, nCmdShow);
}

void Window::InitializeGraphics(HWND hWnd)
{
    UINT flags = 0;
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
    {
        Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
        HRESULT result = CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }

        Microsoft::WRL::ComPtr<ID3D12Device> device;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
        result = D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&device));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }

        D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        result = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Width = 1280;
        swapChainDesc.Height = 760;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> temporarySwapChain;
        result = factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            hWnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &temporarySwapChain);
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }

        result = factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }

        result = temporarySwapChain.As(&m_swapChain);
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
        descriptorHeapDesc.NumDescriptors = 2;
        descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        result = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_descriptorHeap));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }
        m_descriptorHeapSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT n = 0; n < 2; ++n)
        {
            result = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
            if (FAILED(result))
            {
                throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
            }

            device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, descriptorHandle);
            descriptorHandle.ptr += m_descriptorHeapSize;
        }

        result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }

        result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }

        m_commandList->Close();

        result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }

        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            throw std::runtime_error(FORMAT_MESSAGE("CreateEvent failed.\n"));
        }
    }
}

void Window::Update()
{
}

void Window::Render()
{
    HRESULT result = m_commandAllocator->Reset();
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
    }

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    result = m_commandList->Reset(m_commandAllocator.Get(), pipelineState.Get());
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
    }

    {
        D3D12_RESOURCE_BARRIER resourceBarrier = {};
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        resourceBarrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        resourceBarrier.Transition.Subresource = 0;
        resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        m_commandList->ResourceBarrier(1, &resourceBarrier);
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    descriptorHandle.ptr += (m_frameIndex * m_descriptorHeapSize);

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(descriptorHandle, clearColor, 0, nullptr);

    {
        D3D12_RESOURCE_BARRIER resourceBarrier = {};
        resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        resourceBarrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        resourceBarrier.Transition.Subresource = 0;
        resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

        m_commandList->ResourceBarrier(1, &resourceBarrier);
    }

    result = m_commandList->Close();
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
    }

    ID3D12CommandList* commandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, commandLists);

    result = m_swapChain->Present(1, 0);
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
    }

    {
        UINT64 fence = m_fenceValue;
        m_commandQueue->Signal(m_fence.Get(), fence);
        ++m_fenceValue;

        if (m_fence->GetCompletedValue() < fence)
        {
            m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    }
}
