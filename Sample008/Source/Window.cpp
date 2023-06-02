#include "Window.h"

#include "FormatMessage.h"

#include <d3dcompiler.h>
#include <vector>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

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
    CoInitializeEx(0, COINIT_MULTITHREADED);

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
    InitializeResource();

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

        Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
        result = D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }

        D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        result = m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue));
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
        result = m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_descriptorHeap));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }
        m_descriptorHeapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        {
            D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
            srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            srvHeapDesc.NumDescriptors = 1;
            srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            result = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));
            if (FAILED(result))
            {
                throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
            }
        }

        D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT n = 0; n < 2; ++n)
        {
            result = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
            if (FAILED(result))
            {
                throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
            }

            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, descriptorHandle);
            descriptorHandle.ptr += m_descriptorHeapSize;
        }

        result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }

        result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }

        m_commandList->Close();

        result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
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

void Window::InitializeResource()
{
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 uv;
    };
    Vertex vertices[] =
    {
        {{ -0.5f, -0.7f, 0.0f}, {0.0f, 1.0f}},
        {{ -0.5f,  0.7f, 0.0f}, {0.0f, 0.0f}},
        {{  0.5f, -0.7f, 0.0f}, {1.0f, 1.0f}},
        {{  0.5f,  0.7f, 0.0f}, {1.0f, 0.0f}},
    };

    HRESULT result = S_FALSE;
    {
        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeof(vertices);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        result = m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer));
        if (FAILED(result))
        {
            throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
        }
    }

    Vertex* vertexMap = nullptr;
    result = m_vertexBuffer->Map(0, nullptr, (void**)&vertexMap);
    std::copy(std::begin(vertices), std::end(vertices), vertexMap);
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes = sizeof(vertices);
    m_vertexBufferView.StrideInBytes = sizeof(Vertex); 

    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    result = D3DCompileFromFile(
        L"VertexShader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &vsBlob,
        &errorBlob);
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
    }

    result = D3DCompileFromFile(
        L"PixelShader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &psBlob,
        &errorBlob);
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
    }

    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        {
            "POSITION",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0,
        },
        {
            "TEXCOORD",
            0,
            DXGI_FORMAT_R32G32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0,
        },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPiplineStateDesc = {};
    graphicsPiplineStateDesc.pRootSignature = nullptr;
    graphicsPiplineStateDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
    graphicsPiplineStateDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
    graphicsPiplineStateDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
    graphicsPiplineStateDesc.PS.BytecodeLength = psBlob->GetBufferSize();
    graphicsPiplineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    graphicsPiplineStateDesc.RasterizerState.MultisampleEnable = false;
    graphicsPiplineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    graphicsPiplineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    graphicsPiplineStateDesc.RasterizerState.DepthClipEnable = true;
    graphicsPiplineStateDesc.BlendState.AlphaToCoverageEnable = false;
    graphicsPiplineStateDesc.BlendState.IndependentBlendEnable = false;

    D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
    renderTargetBlendDesc.BlendEnable = false;
    renderTargetBlendDesc.LogicOpEnable = false;
    renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    
    graphicsPiplineStateDesc.BlendState.RenderTarget[0] = renderTargetBlendDesc;

    graphicsPiplineStateDesc.InputLayout.pInputElementDescs = inputLayout;
    graphicsPiplineStateDesc.InputLayout.NumElements = _countof(inputLayout);
    graphicsPiplineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    graphicsPiplineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPiplineStateDesc.NumRenderTargets = 1;
    graphicsPiplineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    graphicsPiplineStateDesc.SampleDesc.Count = 1;
    graphicsPiplineStateDesc.SampleDesc.Quality = 0;

    DirectX::TexMetadata metadata = {};
    DirectX::ScratchImage scratchImage = {};

    result = DirectX::LoadFromWICFile(
        L"face.png",
        DirectX::WIC_FLAGS_NONE,
        &metadata,
        scratchImage);
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
    }
    auto image = scratchImage.GetImage(0, 0, 0);
    if (image == nullptr)
    {
        throw std::runtime_error(FORMAT_MESSAGE("GetImage failed."));
    }

    struct TextureRGBA
    {
        unsigned char R, G, B, A;
    };
    std::vector<TextureRGBA> textureData(256 * 256);
    for (auto& rgba : textureData)
    {
        rgba.R = 128;
        rgba.G = 128;
        rgba.B = 0;
        rgba.A = 255;
    }

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
    heapProperties.CreationNodeMask = 0;
    heapProperties.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resourceDesc.Width = metadata.width;
    resourceDesc.Height = static_cast<UINT>(metadata.height);
    resourceDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
    resourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    result = m_device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(&m_textureBuffer));
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
    }
    result = m_textureBuffer->WriteToSubresource(
        0,
        nullptr,
        image->pixels,
        static_cast<UINT16>(image->rowPitch),
        static_cast<UINT16>(image->slicePitch));
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;
    m_device->CreateShaderResourceView(m_textureBuffer.Get(), &shaderResourceViewDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_DESCRIPTOR_RANGE descriptorRange = {};
    descriptorRange.NumDescriptors = 1;
    descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange.BaseShaderRegister = 0;
    descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameter = {};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameter.DescriptorTable.pDescriptorRanges = &descriptorRange;
    rootParameter.DescriptorTable.NumDescriptorRanges = 1;

    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.pParameters = &rootParameter;
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pStaticSamplers = &samplerDesc;
    rootSignatureDesc.NumStaticSamplers = 1;

    Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
    result = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,
        &rootSignatureBlob,
        &errorBlob);
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
    }

    result = m_device->CreateRootSignature(
        0,
        rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature));

    graphicsPiplineStateDesc.pRootSignature = m_rootSignature.Get();

    result = m_device->CreateGraphicsPipelineState(&graphicsPiplineStateDesc, IID_PPV_ARGS(&m_graphicsPiplineState));
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
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

    result = m_commandList->Reset(m_commandAllocator.Get(), m_graphicsPiplineState.Get());
    if (FAILED(result))
    {
        throw std::runtime_error(FORMAT_HRESULT_MESSAGE(result));
    }

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

    D3D12_VIEWPORT viewport = {};
    viewport.Width = 1280;
    viewport.Height = 760;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MaxDepth = 1.0f;
    viewport.MinDepth = 0.0f;
    m_commandList->RSSetViewports(1, &viewport);
    D3D12_RECT scissorRect = {};
    scissorRect.top = 0;
    scissorRect.left = 0;
    scissorRect.right = scissorRect.left + 1280;
    scissorRect.bottom = scissorRect.top + 760;
    m_commandList->RSSetScissorRects(1, &scissorRect);

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
    m_commandList->OMSetRenderTargets(1, &descriptorHandle, FALSE, nullptr);

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(descriptorHandle, clearColor, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->DrawInstanced(4, 1, 0, 0);

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
