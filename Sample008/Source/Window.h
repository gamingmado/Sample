#pragma once

class Window
{
public:
	Window(HINSTANCE hInstance, int nCmdShow);
	virtual ~Window();

	int Run();

private:
	void Initialize(HINSTANCE hInstance, int nCmdShow);
	void InitializeGraphics(HWND hWnd);
	void InitializeResource();
	void Update();
	void Render();

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[2];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_textureBuffer;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_graphicsPiplineState;

	UINT m_frameIndex;
	UINT m_descriptorHeapSize;
	HANDLE m_fenceEvent;
	UINT64 m_fenceValue;
};
