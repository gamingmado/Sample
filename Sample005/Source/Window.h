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
	void Update();
	void Render();

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[2];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT m_frameIndex;
	UINT m_descriptorHeapSize;
	HANDLE m_fenceEvent;
	UINT64 m_fenceValue;
};
