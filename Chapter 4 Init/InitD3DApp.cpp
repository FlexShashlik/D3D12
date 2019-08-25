#include "../Common/d3dApp.h"
#include <DirectXColors.h>

using namespace DirectX;

class InitD3DApp : public D3DApp
{
public:
	InitD3DApp(HINSTANCE hInstance);
	~InitD3DApp();
	
	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	// Run-time memory check for debug
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		InitD3DApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

InitD3DApp::InitD3DApp(HINSTANCE hInstance) : D3DApp(hInstance)
{

}

InitD3DApp::~InitD3DApp()
{

}

bool InitD3DApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	return true;
}

void InitD3DApp::OnResize()
{
	D3DApp::OnResize();
}

void InitD3DApp::Update(const GameTimer& gt)
{

}

void InitD3DApp::Draw(const GameTimer& gt)
{
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCommandList->ResourceBarrier
	(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition
		(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		)
	);

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Clear the back buffer and depth buffer
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Blue, 0, nullptr);
	mCommandList->ClearDepthStencilView
	(
		DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f,
		0,
		0, 
		nullptr
	);

	// Specify the buffers we are going to render to
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mCommandList->ResourceBarrier
	(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition
		(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		)
	);

	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.
	// This waiting is inefficient and is done for simplicity.
	FlushCommandQueue();
}
