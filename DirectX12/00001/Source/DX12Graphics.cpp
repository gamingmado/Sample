﻿#include "DX12Graphics.h"

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
, m_viewport(0.0f, 0.0f, static_cast<float>(window.GetWidth()), static_cast<float>(window.GetHeight()))
, m_scissorRect(0, 0, static_cast<LONG>(window.GetWidth()), static_cast<LONG>(window.GetHeight()))
, m_rootSignature()
, m_vertexBuffer()
, m_vertexBufferView()
{
    Initialize(window);
}

void DX12Graphics::Render()
{
    ResetCommandAllocator();
    ResetCommandList();
    PopulateCommandList();
    CloseCommand();
    ExecuteCommand();
    Present();
    WaitForPreviousFrame();
}

void DX12Graphics::Initialize(Window& window)
{
    UINT flag = 0;
    InitializeDebugLayout(flag);
    InitializeFactory(flag);
    InitializeDevice();
    InitializeCommandQueue();
    InitializeSwapChain(window);
    InitializeDescriptorHeap();
    InitializeRenderTargetView();
    InitializeCommandAllocator();
    InitializeRootSignature();
    InitializeShader();
    InitializeCommandList();
    InitializeFence();
    InitializeFenceEvent();
    InitializeVertexBuffer();
}

void DX12Graphics::InitializeDebugLayout(UINT& flag)
{
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
    HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }

    debugController->EnableDebugLayer();
    flag |= DXGI_CREATE_FACTORY_DEBUG;
}

void DX12Graphics::InitializeFactory(UINT flag)
{
    HRESULT result = CreateDXGIFactory2(flag, IID_PPV_ARGS(&m_factory));
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

void DX12Graphics::InitializeRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }

    result = m_device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::InitializeShader()
{
    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;

    UINT flag = 0;

    HRESULT result = D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", flag, 0, &vertexShaderBlob, nullptr);
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }

    result = D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", flag, 0, &pixelShaderBlob, nullptr);
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPiplineStateDesc = {};
    graphicsPiplineStateDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
    graphicsPiplineStateDesc.pRootSignature = m_rootSignature.Get();
    graphicsPiplineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    graphicsPiplineStateDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    graphicsPiplineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    graphicsPiplineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    graphicsPiplineStateDesc.DepthStencilState.DepthEnable = FALSE;
    graphicsPiplineStateDesc.DepthStencilState.StencilEnable = FALSE;
    graphicsPiplineStateDesc.SampleMask = UINT_MAX;
    graphicsPiplineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPiplineStateDesc.NumRenderTargets = 1;
    graphicsPiplineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    graphicsPiplineStateDesc.SampleDesc.Count = 1;

    result = m_device->CreateGraphicsPipelineState(&graphicsPiplineStateDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::InitializeCommandList()
{
    HRESULT result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList));
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

void DX12Graphics::InitializeVertexBuffer()
{
    Vertex vertices[] =
    {
        {{0.0f, 0.25f, 0.0f}},
        {{0.25f, -0.25f, 0.0f}},
        {{-0.25f, -0.25f, 0.0f}},
    };

    const UINT vertexBufferSize = sizeof(vertices);
    CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

    HRESULT result = m_device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer));

    UINT8* vertexData = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    result = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexData));
    std::memcpy(vertexData, vertices, sizeof(vertices));
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;

    WaitForPreviousFrame();
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
    HRESULT result = m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get());
    if (FAILED(result))
    {
        throw APPLICATION_HRESULT_EXCEPTION(result);
    }
}

void DX12Graphics::PopulateCommandList()
{
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    auto resourceBarrierBegin = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargetViews[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &resourceBarrierBegin);

    CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_descriptorHeapSize);

    m_commandList->OMSetRenderTargets(1, &descriptorHandle, FALSE, nullptr);

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(descriptorHandle, clearColor, 0, nullptr);

    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->DrawInstanced(3, 1, 0, 0);

    auto resourceBarrierEnd = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargetViews[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &resourceBarrierEnd);
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
