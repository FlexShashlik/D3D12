#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	float Time = 0;
};

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	BoxApp(const BoxApp& app) = delete;
	BoxApp& operator=(const BoxApp& app) = delete;
	~BoxApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildPipelineStateObject();

private:
	ComPtr<ID3D12RootSignature> _rootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> _cbvHeap = nullptr; // Constant buffer view's descriptor

	std::unique_ptr<UploadBuffer<ObjectConstants>> _objectCB = nullptr;

	std::unique_ptr<MeshGeometry> _boxGeo = nullptr;

	ComPtr<ID3DBlob> _vsByteCode = nullptr; // Vertex shader
	ComPtr<ID3DBlob> _psByteCode = nullptr; // Pixel shader

	std::vector<D3D12_INPUT_ELEMENT_DESC> _inputLayout;

	ComPtr<ID3D12PipelineState> _PSO = nullptr;

	XMFLOAT4X4 _world = MathHelper::Identity4x4();
	XMFLOAT4X4 _view = MathHelper::Identity4x4();
	XMFLOAT4X4 _proj = MathHelper::Identity4x4();

	float _theta = 1.5f*XM_PI;
	float _phi = XM_PIDIV4;
	float _radius = 5.f;

	POINT _lastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	// Run-time memory check for debug
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		BoxApp theApp(hInstance);
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

BoxApp::BoxApp(HINSTANCE hInstance) : D3DApp(hInstance)
{

}

BoxApp::~BoxApp()
{

}

bool BoxApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to init
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildBoxGeometry();
	BuildPipelineStateObject();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };

	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	// Update the aspect ratio and recompute the projection matrix
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
	XMStoreFloat4x4(&_proj, P);
}

void BoxApp::Update(const GameTimer& gt)
{
	// Convert from spherical to cartesian coords
	float x = _radius * sinf(_phi) * cosf(_theta);
	float y = _radius * sinf(_phi) * sinf(_theta);
	float z = _radius * cosf(_phi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&_view, view);

	XMMATRIX world = XMLoadFloat4x4(&_world);
	XMMATRIX proj = XMLoadFloat4x4(&_proj);
	XMMATRIX worldViewProj = world * view * proj;

	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	objConstants.Time = gt.TotalTime();

	_objectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer& gt)
{
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), _PSO.Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

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

	// Clear the back buffer and depth buffer
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightYellow, 0, nullptr);
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

	ID3D12DescriptorHeap* descriptorHeaps[] = { _cbvHeap.Get() };

	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(_rootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 1, &_boxGeo->VertexBufferView());
	mCommandList->IASetIndexBuffer(&_boxGeo->IndexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCommandList->SetGraphicsRootDescriptorTable(0, _cbvHeap->GetGPUDescriptorHandleForHeapStart());

	mCommandList->DrawIndexedInstanced
	(
		_boxGeo->DrawArgs["box"].IndexCount,
		1, 
		0, 
		0, 
		0
	);

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


void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	_lastMousePos.x = x;
	_lastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - _lastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - _lastMousePos.y));

		// Update angles based on input to orbit camera around box.
		_theta += dx;
		_phi += dy;

		// Restrict the angle mPhi.
		_phi = MathHelper::Clamp(_phi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f*static_cast<float>(x - _lastMousePos.x);
		float dy = 0.005f*static_cast<float>(y - _lastMousePos.y);

		// Update the camera radius based on input.
		_radius += dx - dy;

		// Restrict the radius.
		_radius = MathHelper::Clamp(_radius, 3.0f, 15.0f);
	}

	_lastMousePos.x = x;
	_lastMousePos.y = y;
}

void BoxApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed
	(
		md3dDevice->CreateDescriptorHeap
		(
			&cbvHeapDesc,
			IID_PPV_ARGS(&_cbvHeap)
		)
	);
}

void BoxApp::BuildConstantBuffers()
{
	_objectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = _objectCB->Resource()->GetGPUVirtualAddress();
	// Offset to the ith object constant buffer in the buffer.
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	md3dDevice->CreateConstantBufferView
	(
		&cbvDesc,
		_cbvHeap->GetCPUDescriptorHandleForHeapStart()
	);
}

void BoxApp::BuildRootSignature()
{
	// Shader programs typically require resources as input (constant buffers,
	// textures, samplers).  The root signature defines the resources the shader
	// programs expect.  If we think of the shader programs as a function, and
	// the input resources as function parameters, then the root signature can be
	// thought of as defining the function signature.  

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// Create a single descriptor table of CBVs.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc
	(
		1,
		slotRootParameter,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature
	(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed
	(
		md3dDevice->CreateRootSignature
		(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&_rootSignature)
		)
	);
}

void BoxApp::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	_vsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	_psByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void BoxApp::BuildBoxGeometry()
{
	std::array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) })
	};

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	_boxGeo = std::make_unique<MeshGeometry>();
	_boxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &_boxGeo->VertexBufferCPU));
	CopyMemory(_boxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &_boxGeo->IndexBufferCPU));
	CopyMemory(_boxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	_boxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, _boxGeo->VertexBufferUploader);

	_boxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, _boxGeo->IndexBufferUploader);

	_boxGeo->VertexByteStride = sizeof(Vertex);
	_boxGeo->VertexBufferByteSize = vbByteSize;
	_boxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	_boxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	_boxGeo->DrawArgs["box"] = submesh;
}

void BoxApp::BuildPipelineStateObject()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { _inputLayout.data(), (UINT)_inputLayout.size() };
	psoDesc.pRootSignature = _rootSignature.Get();

	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(_vsByteCode->GetBufferPointer()),
		_vsByteCode->GetBufferSize()
	};

	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(_psByteCode->GetBufferPointer()),
		_psByteCode->GetBufferSize()
	};

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_PSO)));
}
