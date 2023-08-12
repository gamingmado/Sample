#pragma once

namespace Application
{

class Window;

class DX12Graphics
{
public:
    explicit DX12Graphics(Window& window);

    void Render();

private:
    void Initialize(Window& window);
    void InitializeDebugLayout(UINT& flag);
    void InitializeFactory(UINT flag);
    void InitializeDevice();
    void InitializeCommandQueue();
    void InitializeSwapChain(Window& window);
    void InitializeDescriptorHeap();
    void InitializeRenderTargetView();
    void InitializeCommandAllocator();
    void InitializeRootSignature();
    void InitializeShader();
    void InitializeCommandList();
    void InitializeFence();
    void InitializeFenceEvent();
    void InitializeVertexBuffer();
    void ResetCommandAllocator();
    void ResetCommandList();
    void PopulateCommandList();
    void CloseCommand();
    void ExecuteCommand();
    void Present();
    void WaitForPreviousFrame();

private:
    static constexpr std::size_t s_sizeRenderTargetViews = 2;
    static constexpr std::size_t s_sizeCommandList = 1;
    Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, s_sizeRenderTargetViews> m_renderTargetViews;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

    UINT m_frameIndex;
    UINT m_descriptorHeapSize;
    HANDLE m_fenceEvent;
    UINT64 m_fenceValue;

    struct Vertex
    {
        DirectX::XMFLOAT3 position;
    };

    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
};

} // namespace Application
