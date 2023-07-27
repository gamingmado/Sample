#include "DX12Graphics.h"

#include "Window.h"

namespace Application
{

DX12Graphics::DX12Graphics(Window& window)
: m_factory()
, m_device()
, m_commandQueue()
, m_swapChain()
, m_descriptorHeap()
, m_renderTargetViews()
, m_commandAllocator()
, m_commandList()
, m_fence()
, m_frameIndex(0)
, m_descriptorHeapSize(0)
, m_fenceEvent(nullptr)
, m_fenceValue(0)
{
    InitializeFactory();
    InitializeDevice();
    InitializeCommandQueue();
    InitializeSwapChain(window);
    InitializeDescriptorHeap();
    InitializeRenderTargetView();
    InitializeCommandAllocator();
    InitializeCommandList();
    InitializeFence();
    InitializeFenceEvent();
}

void DX12Graphics::Render()
{
    ResetCommandAllocator();
    ResetCommandList();
    BeginBarrier();
    ClearRenderTargetView();
    EndBarrier();
    CloseCommand();
    ExecuteCommand();
    Present();
    WaitForPreviousFrame();
}

void DX12Graphics::InitializeFactory()
{
    UINT flags = 0;
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        flags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    HRESULT result = CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory));
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::InitializeDevice()
{
    HRESULT result = D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device));
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::InitializeCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    HRESULT result = m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue));
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::InitializeSwapChain(Window& window)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = static_cast<UINT>(m_renderTargetViews.size());
    swapChainDesc.Width = window.GetWidth();
    swapChainDesc.Height = window.GetHeight();
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    HRESULT result = m_factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        window.GetHandle(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain);
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }

    result = m_factory->MakeWindowAssociation(window.GetHandle(), DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }

    result = swapChain.As(&m_swapChain);
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void DX12Graphics::InitializeDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = static_cast<UINT>(m_renderTargetViews.size());
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT result = m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_descriptorHeap));
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }

    m_descriptorHeapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void DX12Graphics::InitializeRenderTargetView()
{
    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    auto size = m_renderTargetViews.size();
    for (std::size_t i = 0; i < size; ++i)
    {
        HRESULT result = m_swapChain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&m_renderTargetViews[i]));
        if (FAILED(result))
        {
            throw APPLICATION_HRESULT_EXCEPTION(result);
        }

        m_device->CreateRenderTargetView(m_renderTargetViews[i].Get(), nullptr, descriptorHandle);
        descriptorHandle.ptr += m_descriptorHeapSize;
    }
}

void DX12Graphics::InitializeCommandAllocator()
{
    HRESULT result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::InitializeCommandList()
{
    HRESULT result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }

    m_commandList->Close();
}

void DX12Graphics::InitializeFence()
{
    HRESULT result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::InitializeFenceEvent()
{
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
    {
        throw APPLICATION_EXCEPTION("CreateEvent failed.");
    }
}

void DX12Graphics::ResetCommandAllocator()
{
    HRESULT result = m_commandAllocator->Reset();
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::ResetCommandList()
{
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
    HRESULT result = m_commandList->Reset(m_commandAllocator.Get(), pipelineState.Get());
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::BeginBarrier()
{
    D3D12_RESOURCE_BARRIER resourceBarrier = {};
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier.Transition.pResource = m_renderTargetViews[m_frameIndex].Get();
    resourceBarrier.Transition.Subresource = 0;
    resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    m_commandList->ResourceBarrier(1, &resourceBarrier);
}

void DX12Graphics::ClearRenderTargetView()
{
    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    descriptorHandle.ptr += (m_frameIndex * m_descriptorHeapSize);

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(descriptorHandle, clearColor, 0, nullptr);
}

void DX12Graphics::EndBarrier()
{
    D3D12_RESOURCE_BARRIER resourceBarrier = {};
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier.Transition.pResource = m_renderTargetViews[m_frameIndex].Get();
    resourceBarrier.Transition.Subresource = 0;
    resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    m_commandList->ResourceBarrier(1, &resourceBarrier);
}

void DX12Graphics::CloseCommand()
{
    HRESULT result = m_commandList->Close();
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::ExecuteCommand()
{
    ID3D12CommandList* commandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(1, commandLists);
}

void DX12Graphics::Present()
{
    HRESULT result = m_swapChain->Present(1, 0);
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::WaitForPreviousFrame()
{
    UINT64 fence = m_fenceValue;
    HRESULT result = m_commandQueue->Signal(m_fence.Get(), fence);
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
    ++m_fenceValue;

    if (m_fence->GetCompletedValue() < fence)
    {
        result = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
        if (FAILED(result))
        {
            throw APPLICATION_HRESULT_EXCEPTION(result);
        }
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

} // namespace Application
