//-----------------------------------------------------------------------------
// File: CGameFramework.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "GameFramework.h"
#include "Network.h"
#include "CMonster.h"
#include "SoundManager.h"

CGameFramework::CGameFramework()
{
	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	for (int i = 0; i < m_nSwapChainBuffers; i++) m_ppd3dSwapChainBackBuffers[i] = NULL;
	m_nSwapChainBufferIndex = 0;

	m_pd3dCommandAllocator = NULL;
	m_pd3dCommandQueue = NULL;
	m_pd3dCommandList = NULL;

	m_pd3dRtvDescriptorHeap = NULL;
	m_pd3dDsvDescriptorHeap = NULL;

	m_hFenceEvent = NULL;
	m_pd3dFence = NULL;
	for (int i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_nWndClientWidth = FRAME_BUFFER_WIDTH;
	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;

	m_pScene = NULL;
	m_pPlayer = NULL;

	m_nPendingScene = -1;

	_tcscpy_s(m_pszFrameRate, _T("Monster Breakers "));
}

CGameFramework::~CGameFramework()
{}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvDescriptorHeaps();
	CreateSwapChain();
	CreateDepthStencilView();

	CoInitialize(NULL);

	CSoundManager::GetInstance()->Init();

	BuildObjects();
	//LoadingDoneToServer();

	return(true);
}

void CGameFramework::CreateSwapChain()
{
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);
	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

#ifdef _WITH_CREATE_SWAPCHAIN_FOR_HWND
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Windowed = TRUE;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue, m_hWnd, &dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1**)&m_pdxgiSwapChain);
#else
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.OutputWindow = m_hWnd;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.Windowed = TRUE;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain**)&m_pdxgiSwapChain);
#endif
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	hResult = m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	CreateRenderTargetViews();
#endif
}

void CGameFramework::CreateDirect3DDevice()
{
	HRESULT hResult;

	UINT nDXGIFactoryFlags = 0;
#if defined(_DEBUG)
	ID3D12Debug* pd3dDebugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&pd3dDebugController);
	if (pd3dDebugController)
	{
		pd3dDebugController->EnableDebugLayer();
		pd3dDebugController->Release();
	}
	nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	hResult = ::CreateDXGIFactory2(nDXGIFactoryFlags, __uuidof(IDXGIFactory4), (void**)&m_pdxgiFactory);

	IDXGIAdapter1* pd3dAdapter = NULL;

	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void**)&m_pd3dDevice))) break;
	}

	if (!pd3dAdapter)
	{
		m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIFactory4), (void**)&pd3dAdapter);
		hResult = D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void**)&m_pd3dDevice);
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	hResult = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_pd3dFence);
	for (UINT i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	::gnCbvSrvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	::gnRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	::gnDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	if (pd3dAdapter) pd3dAdapter->Release();
}

void CGameFramework::CreateCommandQueueAndList()
{
	HRESULT hResult;

	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void**)&m_pd3dCommandQueue);

	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_pd3dCommandAllocator);

	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_pd3dCommandList);
	hResult = m_pd3dCommandList->Close();
}

void CGameFramework::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dRtvDescriptorHeap);
	::gnRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dDsvDescriptorHeap);
	::gnDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CGameFramework::CreateRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
	{
		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&m_ppd3dSwapChainBackBuffers[i]);
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dSwapChainBackBuffers[i], NULL, d3dRtvCPUDescriptorHandle);
		d3dRtvCPUDescriptorHandle.ptr += ::gnRtvDescriptorIncrementSize;
	}
}

void CGameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void**)&m_pd3dDepthStencilBuffer);

	D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc;
	::ZeroMemory(&d3dDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	d3dDepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dDepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, &d3dDepthStencilViewDesc, d3dDsvCPUDescriptorHandle);
}

void CGameFramework::ChangeSwapChainState()
{
	WaitForGpuComplete();

	BOOL bFullScreenState = FALSE;
	m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL);
	m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, NULL);

	DXGI_MODE_DESC dxgiTargetParameters;
	dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiTargetParameters.Width = m_nWndClientWidth;
	dxgiTargetParameters.Height = m_nWndClientHeight;
	dxgiTargetParameters.RefreshRate.Numerator = 60;
	dxgiTargetParameters.RefreshRate.Denominator = 1;
	dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters);

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, m_nWndClientWidth, m_nWndClientHeight, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene) m_pScene->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);

	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		::SetCapture(hWnd);
		::GetCursorPos(&m_ptOldCursorPos);
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		::ReleaseCapture();
		break;
	case WM_MBUTTONDOWN:
		// 가운데 버튼을 누르고 있는 동안에만 시점(카메라)을 회전시킨다.
		// 좌/우클릭(공격, 스킬강화 UI 등)에는 전혀 영향을 주지 않는다.
		m_bMouseOrbitDragging = true;
		::SetCapture(hWnd);
		::GetCursorPos(&m_ptOrbitLastPos);
		break;
	case WM_MBUTTONUP:
		m_bMouseOrbitDragging = false;
		::ReleaseCapture();
		break;
	case WM_MOUSEMOVE:
		if (m_bMouseOrbitDragging && m_pPlayer)
		{
			POINT ptCurrent;
			::GetCursorPos(&ptCurrent);
			int nDeltaX = ptCurrent.x - m_ptOrbitLastPos.x;
			int nDeltaY = ptCurrent.y - m_ptOrbitLastPos.y;

			if (nDeltaX != 0 || nDeltaY != 0)
			{
				CCamera* pCamera = m_pPlayer->GetCamera();
				if (pCamera && (pCamera->GetMode() == THIRD_PERSON_CAMERA))
				{
					CThirdPersonCamera* p3rdPersonCamera = (CThirdPersonCamera*)pCamera;

					const float fOrbitSensitivity = 0.15f; // 도(degree) / 픽셀
					float fDeltaYaw = nDeltaX * fOrbitSensitivity;
					float fDeltaPitch = -nDeltaY * fOrbitSensitivity; // 마우스를 위로 올리면 위를 보도록 부호 반전

					p3rdPersonCamera->AddOrbitRotation(fDeltaYaw, fDeltaPitch);
				}
			}
			m_ptOrbitLastPos = ptCurrent;
		}
		::SetCapture(hWnd);
		::GetCursorPos(&m_ptOldCursorPos);
		break;
	default:
		break;
	}

}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene) m_pScene->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
	if (!isStartScene) {
		switch (nMessageID)
		{
		case WM_KEYUP:
			switch (wParam)
			{
			case VK_ESCAPE:
				exit(0);
				break;
			case VK_F1:
			case VK_F2:
			case VK_F3:
				m_pCamera = m_pPlayer->ChangeCamera((DWORD)(wParam - VK_F1 + 1), m_GameTimer.GetTimeElapsed());
				break;
			case VK_F9:
				ChangeSwapChainState();
				break;
			}
			break;
		default:
			break;
		}
	}
}

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) == WA_INACTIVE)
			m_GameTimer.Stop();
		else
			m_GameTimer.Start();
		break;
	}
	case WM_SIZE:
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
		OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;
	}
	return(0);
}


// -------------------------------------------------------------------------------------------


void CGameFramework::OnDestroy()
{
	ReleaseObjects();

	::CloseHandle(m_hFenceEvent);

	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap->Release();

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap->Release();

	if (m_pd3dCommandAllocator) m_pd3dCommandAllocator->Release();
	if (m_pd3dCommandQueue) m_pd3dCommandQueue->Release();
	if (m_pd3dCommandList) m_pd3dCommandList->Release();

	if (m_pd3dFence) m_pd3dFence->Release();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain) m_pdxgiSwapChain->Release();
	if (m_pd3dDevice) m_pd3dDevice->Release();
	if (m_pdxgiFactory) m_pdxgiFactory->Release();

#if defined(_DEBUG)
	IDXGIDebug1* pdxgiDebug = NULL;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&pdxgiDebug);
	HRESULT hResult = pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	pdxgiDebug->Release();
#endif
}

#define _WITH_TERRAIN_PLAYER

void CGameFramework::BuildObjects()
{
	isLoading = true;

	m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	m_nScenes = 4; // 총 Scene 개수
	if (!m_ppScenes) {
		m_ppScenes = new CScene * [m_nScenes] {};  // 딱 한 번만 할당, NULL로 초기화
	}

	LoadSoundResources();

	bool b = false;
	if (m_nCurrentScene == 0) {
		m_ppScenes[0] = new CStartScene();
		m_ppScenes[0]->BuildObjects(m_pd3dDevice, m_pd3dCommandList);
		CTerrainPlayer* pPlayer = new CTerrainPlayer(m_pd3dDevice, m_pd3dCommandList, m_ppScenes[0]->GetGraphicsRootSignature(), NULL, NULL);
		m_ppScenes[0]->SetPlayer(pPlayer);
	}
	else if (m_nCurrentScene == 1) {
		m_ppScenes[1] = new CSelectScene();
		m_ppScenes[1]->BuildObjects(m_pd3dDevice, m_pd3dCommandList);
		CTerrainPlayer* pPlayer = new CTerrainPlayer(m_pd3dDevice, m_pd3dCommandList, m_ppScenes[1]->GetGraphicsRootSignature(), NULL, NULL);
		m_ppScenes[1]->SetPlayer(pPlayer);
	}
	else if (m_nCurrentScene == 2) {
		m_ppScenes[2] = new CScene();
		m_ppScenes[2]->BuildObjects(m_pd3dDevice, m_pd3dCommandList);
		if (GetSelectedPlayerModel() == EPlayerModelType::Wizard)
			m_ppScenes[2]->m_pModel = m_ppScenes[2]->m_pWizardModel;
		else if (GetSelectedPlayerModel() == EPlayerModelType::Knight)
			m_ppScenes[2]->m_pModel = m_ppScenes[2]->m_pKnightModel;
		else
			m_ppScenes[2]->m_pModel = m_ppScenes[2]->m_pThiefModel;
		m_ppScenes[2]->BuildSimpleUI(m_pd3dDevice, m_pd3dCommandList);

		CTerrainPlayer* pPlayer = new CTerrainPlayer(m_pd3dDevice, m_pd3dCommandList, m_ppScenes[2]->GetGraphicsRootSignature(), m_ppScenes[2]->m_pTerrain, m_ppScenes[2]->m_pModel);

		m_ppScenes[2]->SetPlayer(pPlayer);
		for (auto* monster : m_ppScenes[2]->m_Monsters) {
			monster->SetPlayer(pPlayer);
		}
		//m_pPlayer->SetPosition(XMFLOAT3(3, 0, 20));

		m_ppScenes[2]->GenerateGameObjectsBoundingBox();
		m_ppScenes[2]->InitializeCollisionSystem();
	}
	else if (m_nCurrentScene == 3) {
		m_ppScenes[4] = new CEndScene();
		m_ppScenes[4]->BuildObjects(m_pd3dDevice, m_pd3dCommandList);
		CPlayer* pPlayer = new CPlayer();
		m_ppScenes[4]->SetPlayer(pPlayer);
	}

	//#ifdef _WITH_TERRAIN_PLAYER
	//	CTerrainPlayer *pPlayer = new CTerrainPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), m_pScene->m_pTerrain);
	//#else
	//	CAirplanePlayer *pPlayer = new CAirplanePlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), NULL);
	//	pPlayer->SetPosition(XMFLOAT3(425.0f, 240.0f, 640.0f));
	//#endif

	m_nScene = m_nCurrentScene; // 현재 활성화 Scene 인덱스
	m_pScene = m_ppScenes[m_nScene];
	m_pScene->m_pPlayer = m_pPlayer = m_pScene->GetPlayer();
	m_pCamera = m_pPlayer->ChangeCamera(THIRD_PERSON_CAMERA, m_GameTimer.GetTimeElapsed());

	// 공격을 위한 몬스터의 플레이어 세팅
	//if (m_nCurrentScene == 1)
	//	m_pScene->m_ppMonsters[0]->SetPlayer(m_pPlayer);


	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();

	m_GameTimer.Reset();
	isLoading = false;
}

void CGameFramework::LoadSoundResources()
{
	// =================================================================
	// SFX (효과음)
	// =================================================================

	// --- 공통 행동 (Common) ---
	CSoundManager::GetInstance()->LoadSound("player_die", "Sound/player_die.mp3", false);//
	CSoundManager::GetInstance()->LoadSound("player_respawn", "Sound/player_respawn.mp3", false);//
	CSoundManager::GetInstance()->LoadSound("player_hurt", "Sound/player_hurt.mp3", false);//

	// --- 발소리 (Footsteps) ---
	CSoundManager::GetInstance()->LoadSound("footstep_sand_1", "Sound/footstep_sand_1.mp3", false);
	CSoundManager::GetInstance()->LoadSound("footstep_sand_2", "Sound/footstep_sand_2.mp3", false);
	CSoundManager::GetInstance()->LoadSound("footstep_sand_3", "Sound/footstep_sand_3.mp3", false);
	CSoundManager::GetInstance()->LoadSound("Footstep01", "Sound/Footstep01.wav", false);
	CSoundManager::GetInstance()->LoadSound("Footstep02", "Sound/Footstep02.wav", false);
	CSoundManager::GetInstance()->LoadSound("Footstep03", "Sound/Footstep03.wav", false);
	CSoundManager::GetInstance()->LoadSound("Footstep04", "Sound/Footstep04.wav", false);
	CSoundManager::GetInstance()->LoadSound("walk_on_grass_1", "Sound/walk_on_grass_1.mp3", false);
	CSoundManager::GetInstance()->LoadSound("walk_on_grass_2", "Sound/walk_on_grass_2.mp3", false);
	CSoundManager::GetInstance()->LoadSound("walk_on_grass_3", "Sound/walk_on_grass_3.mp3", false);

	// --- 기사 (Knight) ---
	CSoundManager::GetInstance()->LoadSound("knight_attack_1", "Sound/knight_attack_1.mp3", false);
	CSoundManager::GetInstance()->LoadSound("knight_attack_2", "Sound/knight_attack_2.mp3", false);
	CSoundManager::GetInstance()->LoadSound("knight_e", "Sound/knight_e.mp3", false);
	CSoundManager::GetInstance()->LoadSound("knight_hit", "Sound/knight_hit.mp3", false);
	CSoundManager::GetInstance()->LoadSound("knight_q", "Sound/knight_q.mp3", false);
	CSoundManager::GetInstance()->LoadSound("knight_rk_1", "Sound/knight_rk_1.mp3", false);
	CSoundManager::GetInstance()->LoadSound("knight_rk_2", "Sound/knight_rk_2.mp3", false);

	// --- 도적 (Rogue) ---
	CSoundManager::GetInstance()->LoadSound("rogue_attack", "Sound/rogue_attack.mp3", false);
	CSoundManager::GetInstance()->LoadSound("rogue_e", "Sound/rogue_e.mp3", false);
	CSoundManager::GetInstance()->LoadSound("rogue_q", "Sound/rogue_q.mp3", false);
	CSoundManager::GetInstance()->LoadSound("rogue_rk", "Sound/rogue_rk.mp3", false);

	// --- 마법사 (Wizard) ---
	CSoundManager::GetInstance()->LoadSound("wizard_attack", "Sound/wizard_attack.mp3", false);
	CSoundManager::GetInstance()->LoadSound("wizard_e", "Sound/wizard_e.mp3", false);
	CSoundManager::GetInstance()->LoadSound("wizard_q", "Sound/wizard_q.mp3", false);
	CSoundManager::GetInstance()->LoadSound("wizard_rk", "Sound/wizard_rk.mp3", false);

	// --- 몬스터 & 보스 (Monster & Boss) ---
	CSoundManager::GetInstance()->LoadSound("monster_hurt_1", "Sound/monster_hurt_1.mp3", false);
	CSoundManager::GetInstance()->LoadSound("monster_hurt_2", "Sound/monster_hurt_2.mp3", false);
	CSoundManager::GetInstance()->LoadSound("monster_die_2", "Sound/monster_die_1.mp3", false);
	CSoundManager::GetInstance()->LoadSound("monster_die_2", "Sound/monster_die_2.mp3", false);
	CSoundManager::GetInstance()->LoadSound("boss_attack_1", "Sound/boss_attack_1.mp3", false);
	CSoundManager::GetInstance()->LoadSound("boss_attack_2", "Sound/boss_attack_2.mp3", false);
	CSoundManager::GetInstance()->LoadSound("boss_attack_3", "Sound/boss_attack_3.mp3", false);
	CSoundManager::GetInstance()->LoadSound("boss_hurt", "Sound/boss_hurt.mp3", false);
	CSoundManager::GetInstance()->LoadSound("boss_walk", "Sound/boss_walk.mp3", false);
	CSoundManager::GetInstance()->LoadSound("boss_die_1", "Sound/boss_die_1.mp3", false);
	CSoundManager::GetInstance()->LoadSound("boss_die_2", "Sound/boss_die_2.mp3", false);


	// =================================================================
	// BGM
	// =================================================================
	CSoundManager::GetInstance()->LoadSound("bgm_bossstage", "Sound/bgm_bossstage.wav", true);
	CSoundManager::GetInstance()->LoadSound("bgm_login", "Sound/bgm_login.mp3", true);
	CSoundManager::GetInstance()->LoadSound("bgm_village", "Sound/bgm_village.mp3", true);
	CSoundManager::GetInstance()->LoadSound("bgm_ending", "Sound/bgm_ending.mp3", true);
	CSoundManager::GetInstance()->LoadSound("bgm_battle", "Sound/bgm_battle.mp3", true);
	CSoundManager::GetInstance()->LoadSound("bgm_winner", "Sound/bgm_winner.mp3", true);
}

void CGameFramework::ReleaseObjects()
{
	if (m_pPlayer) m_pPlayer->Release();

	if (m_pScene) m_pScene->ReleaseObjects();
	if (m_pScene) delete m_pScene;
}

void CGameFramework::ProcessInput()
{
	static UCHAR pKeysBuffer[256];

	static bool bPrevSpace = false;
	static float fRemainingYaw = 0.0f;

	bool bProcessedByScene = false;
	if (GetKeyboardState(pKeysBuffer) && m_pScene) bProcessedByScene = m_pScene->ProcessInput(pKeysBuffer);
	if (!bProcessedByScene)
	{
		m_pScene->m_ptPos = m_ptOldCursorPos;
		DWORD dwDirection = 0;
		if (pKeysBuffer['W'] & 0xF0) dwDirection |= DIR_FORWARD;
		if (pKeysBuffer['S'] & 0xF0) dwDirection |= DIR_BACKWARD;
		if (pKeysBuffer[VK_SPACE] & 0xF0) dwDirection |= DIR_UP;
		if (pKeysBuffer[VK_SHIFT] & 0xF0) dwDirection |= DIR_DOWN;
		bool bCurrA = (pKeysBuffer['A'] & 0xF0);
		bool bCurrD = (pKeysBuffer['D'] & 0xF0);

		CTerrainPlayer* terrainPlayer = dynamic_cast<CTerrainPlayer*>(m_pPlayer);
		if (!terrainPlayer) return;

		bool bForward = (dwDirection & DIR_FORWARD) != 0;

		float fTimeElapsed = m_GameTimer.GetTimeElapsed();

		const float fTurnSpeed = 180.0f; // 180~360 사이로 취향 조절

		float yawInput = 0.0f;
		if (bCurrA) yawInput -= 1.0f;
		if (bCurrD) yawInput += 1.0f;

		if (yawInput != 0.0f)
		{
			terrainPlayer->Rotate(0.0f, yawInput * fTurnSpeed * fTimeElapsed, 0.0f);
		}

		AnimationState currentState = terrainPlayer->m_currentAnim;

		if (currentState != AnimationState::ATTACK &&
			currentState != AnimationState::SKILL1 &&
			currentState != AnimationState::SKILL2 &&
			currentState != AnimationState::SKILL3)
		{

			bool isMoving = dwDirection & (DIR_FORWARD | DIR_BACKWARD);
			bool isRunning = dwDirection & DIR_DOWN;

			if (isRunning && isMoving)
			{
				terrainPlayer->m_currentAnim = AnimationState::RUN;
				terrainPlayer->Move(dwDirection, 3.5f, true);
			}
			else if (isMoving)
			{
				terrainPlayer->m_currentAnim = AnimationState::WALK;
				terrainPlayer->Move(dwDirection, 3.5f, true);
			}
			else
			{
				terrainPlayer->m_currentAnim = AnimationState::IDLE;
			}
		}
	}
	m_pPlayer->Update(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::AnimateObjects()
{
	float fTimeElapsed = m_GameTimer.GetTimeElapsed();

	if (m_pScene) {
		m_pScene->AnimateObjects(fTimeElapsed);
		if (m_pScene->m_vPlayers.size() > 0)
			for (auto& kv : g_other_player_slots)
			{
				long long player_id = kv.first;
				int slot = kv.second;
				OtherPlayer* otherPlayer = m_pScene->m_ppOtherPlayers[slot];
				if (!otherPlayer) continue;
				/*				cout << "[RENDER] id=" << player_id << " slot=" << slot
									<< " pos=(" << otherPlayer->m_xmf3Position.x << ", "
									<< otherPlayer->m_xmf3Position.y << ", "
									<< otherPlayer->m_xmf3Position.z << ")\n";*/
				m_pScene->m_ppOtherPlayers[slot]->Animate(otherPlayer->targetAnim, fTimeElapsed);
			}
		if (!isLoading && !isStartScene) {
			for (auto& [id, pMonster] : g_monsters) {
				if (pMonster) {
					pMonster->Animate(fTimeElapsed);
				}
			}
		}
	}

	m_pPlayer->Animate(fTimeElapsed);

	if (m_nCurrentScene == 0) m_pPlayer->SetPosition(XMFLOAT3(3, 0, 20));

}

void CGameFramework::WaitForGpuComplete()
{
	const UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MoveToNextScene(int i)
{
	int prev = m_nCurrentScene;
	if (prev >= 0 && prev < 4 && m_ppScenes[prev])
	{
		m_ppScenes[prev]->ReleaseObjects();
		delete m_ppScenes[prev];
		m_ppScenes[prev] = nullptr;
	}
	m_nCurrentScene = i;
	BuildObjects();
	LoadingDoneToServer();
	if (i == 2) isStartScene = false;
}

//#define _WITH_PLAYER_TOP

void CGameFramework::FrameAdvance()
{
	if (m_nPendingScene != -1)
	{
		int next = m_nPendingScene;
		m_nPendingScene = -1;

		MoveToNextScene(next);
		return;
	}

	m_GameTimer.Tick(60.0f);
	HRESULT hResult = m_pd3dCommandAllocator->Reset();
	hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex];
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * ::gnRtvDescriptorIncrementSize);

	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	m_pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor/*Colors::Azure*/, 0, NULL);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	m_pd3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);

	if (!isStartScene) ProcessInput();
	AnimateObjects();

	//WaitForGpuComplete();
	if (m_pScene) m_pScene->UpdateUI(m_pd3dCommandList);
	if (m_pScene) m_pScene->Render(m_pd3dCommandList, m_pCamera, d3dRtvCPUDescriptorHandle, d3dDsvCPUDescriptorHandle);

#ifdef _WITH_PLAYER_TOP
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
#endif
	if (m_pPlayer && !isStartScene) m_pPlayer->Render(m_pd3dCommandList, m_pCamera);

	CSoundManager::GetInstance()->Update();

	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	hResult = m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

#ifdef _WITH_PRESENT_PARAMETERS
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
	dxgiPresentParameters.DirtyRectsCount = 0;
	dxgiPresentParameters.pDirtyRects = NULL;
	dxgiPresentParameters.pScrollRect = NULL;
	dxgiPresentParameters.pScrollOffset = NULL;
	m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);
#else
#ifdef _WITH_SYNCH_SWAPCHAIN
	m_pdxgiSwapChain->Present(1, 0);
#else
	m_pdxgiSwapChain->Present(0, 0);
#endif
#endif
	WaitForGpuComplete();

	MoveToNextFrame();

	m_GameTimer.GetFrameRate(m_pszFrameRate + 18, 37);
	size_t nLength = _tcslen(m_pszFrameRate);
	std::wstring w_user_name(user_name.begin(), user_name.end());
	XMFLOAT3 vPos = m_pPlayer->GetPosition();
	_stprintf_s(m_pszFrameRate + nLength, 100 - nLength,
		_T(" - ID : %s | Pos: X:%.1f, Y:%.1f, Z:%.1f"),
		w_user_name.c_str(), vPos.x, vPos.y, vPos.z);
	::SetWindowText(m_hWnd, m_pszFrameRate);
}



void CGameFramework::ItemSpawned(long long itemID, const XMFLOAT3& pos, int type, int price)
{
	auto it = g_items.find(itemID);
	if (it != g_items.end())
	{
		it->second->SetPosition(pos);
		it->second->SetPrice(price);
		UpdateItemPosition(itemID, pos);
	}
	else
	{

	}
}

void CGameFramework::UpdateItemPosition(long long itemID, const XMFLOAT3& pos)
{
	auto it = g_items.find(itemID);
	if (it != g_items.end())
	{
		it->second->SetPosition(pos);
	}
}

void CGameFramework::UpdateItemRotation(long long itemID, const XMFLOAT3& look, const XMFLOAT3& right)
{
	auto it = g_items.find(itemID);
	if (it != g_items.end())
	{
		it->second->SetLookVector(look);
		it->second->SetRightVector(right);
	}
}


//void CGameFramework::OnMonsterSpawned(int monsterID, const XMFLOAT3& pos, int state)
//{
//	auto it = g_monsters.find(monsterID);
//	if (it != g_monsters.end())
//	{
//
//		// 기존 몬스터 위치/상태/HP 갱신
//		it->second->SetPosition(pos);
//		UpdateMonsterState(it->second, state);
//
//		cout << "[OnMonsterSpawned] ID=" << monsterID << " pos=(" << pos.x << "," << pos.y << "," << pos.z << ") 갱신\n";
//	}
//	else
//	{
//		//// 몬스터 객체 새로 생성 (생성자 파라미터는 적절히 수정)
//		//CSpider* pMonster = new CSpider(pd3dDevice, pd3dCommandList, pRootSignature, pModel, 5);
//		//pMonster->SetPosition(pos);
//		//UpdateMonsterState(pMonster, state);
//
//
//		//g_monsters[monsterID] = pMonster;
//
//		//// 씬에서 관리하는 리스트나 배열에도 추가할 수 있음
//
//
//
//	}
//}

void CGameFramework::OnMonsterSpawned(long long monsterID, const XMFLOAT3& pos, int state)
{
	auto it = g_monsters.find(monsterID);
	if (it != g_monsters.end())
	{
		// ── 기존: 이미 등록된 몬스터 위치/상태 갱신 (리스폰 포함) ──
		CMonster* pMonster = it->second;

		// 죽었다가 다시 스폰되는 경우 → 상태 초기화
		if (pMonster->IsDead())
		{
			pMonster->ResetHP();
		}

		pMonster->SetPosition(pos);
		UpdateMonsterState(pMonster, state);

		cout << "[OnMonsterSpawned] 갱신 ID=" << monsterID
			<< " pos=(" << pos.x << "," << pos.y << "," << pos.z << ")\n";
	}
	else
	{
		// ── 새 ID: MONSTER_DESCS 테이블로 모델 파일·HP 조회 후 동적 생성 ──

		// 서버 ID 규칙: startID, startID+1, startID+2 → 3마리씩 묶음
		// MONSTER_DESCS[i].startID 기준으로 어느 종류인지 역추산
		const MonsterDesc* pDesc = nullptr;
		for (const auto& desc : MONSTER_DESCS)
		{
			const long long offset = monsterID - desc.startID;
			if (offset >= 0 && offset < 3)   // 3마리 묶음
			{
				pDesc = &desc;
				break;
			}
		}

		if (!pDesc)
		{
			cout << "[OnMonsterSpawned] 알 수 없는 ID=" << monsterID << " → 스킵\n";
			return;
		}

		// CommandList 사용을 위해 Reset 필요 (ResetCommandList 헬퍼가 있으면 활용)
		m_pd3dCommandAllocator->Reset();
		m_pd3dCommandList->Reset(m_pd3dCommandAllocator, nullptr);

		CMonster* pMonster = new CMonster(
			m_pd3dDevice,
			m_pd3dCommandList,
			m_ppScenes[m_nCurrentScene]->GetGraphicsRootSignature(),
			pDesc->modelPath,
			5,          // nAnimationTracks: Idle/Walk/Attack/GetHit/Death
			nullptr,    // pModel: nullptr → 내부에서 파일 로드
			pDesc->hp,
			monsterID
		);

		pMonster->SetPosition(pos);
		if (pDesc->scale != 1.0f)
			pMonster->SetScale(pDesc->scale, pDesc->scale, pDesc->scale);

		// 플레이어 참조 연결 (HP바 LookAt용)
		CScene* pScene = m_ppScenes[m_nCurrentScene];
		if (pScene && pScene->m_pPlayer)
			pMonster->SetPlayer(pScene->m_pPlayer);

		UpdateMonsterState(pMonster, state);

		// Scene의 m_Monsters 벡터에 추가 (Render/Animate 루프에 포함됨)
		pScene->m_Monsters.push_back(pMonster);

		// Upload Buffer 해제 (GPU 업로드 완료 후)
		m_pd3dCommandList->Close();
		ID3D12CommandList* ppCmdLists[] = { m_pd3dCommandList };
		m_pd3dCommandQueue->ExecuteCommandLists(1, ppCmdLists);
		WaitForGpuComplete();   // 기존 헬퍼 함수 사용

		pMonster->ReleaseUploadBuffers();

		cout << "[OnMonsterSpawned] 새 몬스터 생성 ID=" << monsterID
			<< " 모델=" << pDesc->modelPath
			<< " pos=(" << pos.x << "," << pos.y << "," << pos.z << ")\n";
	}
}

void CGameFramework::UpdateMonsterState(CMonster* pMonster, int state)
{
	// 애니메이션 트랙 설정 등
	for (int i = 0; i < 5; ++i)
		pMonster->m_pSkinnedAnimationController->SetTrackEnable(i, false);

	switch (state)
	{
	case 0: pMonster->m_pSkinnedAnimationController->SetTrackEnable(0, true); break; // idle
	case 1: pMonster->m_pSkinnedAnimationController->SetTrackEnable(1, true); break; // walk
	case 2: pMonster->m_pSkinnedAnimationController->SetTrackEnable(2, true); break; // attack
	case 3: pMonster->m_pSkinnedAnimationController->SetTrackEnable(3, true); break; // gethit
	case 4: pMonster->m_pSkinnedAnimationController->SetTrackEnable(4, true); break; // death
	default: break;
	}
}

void CGameFramework::UpdateMonsterPosition(long long monsterID, const XMFLOAT3& pos, const XMFLOAT3& rot, int state)
{
	auto it = g_monsters.find(monsterID);
	if (it == g_monsters.end())
	{
		std::cout << "[Error] Monster ID not found: " << monsterID << std::endl;
		return;
	}
	/*	printf("[Net] Monster %d → pos=(%.1f, %.1f, %.1f) state=%d\n",
			monsterID, pos.x, pos.y, pos.z, state);*/
	CMonster* pMonster = it->second;
	pMonster->SetPosition(pos);
	//pMonster->CalculateBoundingBox();

	//pMonster->Rotate(rot);
	float len = sqrtf(rot.x * rot.x + rot.z * rot.z);
	if (len > 0.001f)
	{
		XMFLOAT3 look = { rot.x / len, 0.0f, rot.z / len };  // XZ 정규화
		XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };

		// right = up × look
		XMFLOAT3 right = {
			 up.y * look.z - up.z * look.y,
			 up.z * look.x - up.x * look.z,
			 up.x * look.y - up.y * look.x
		};

		// right 정규화
		float rlen = sqrtf(right.x * right.x + right.y * right.y + right.z * right.z);
		if (rlen > 0.001f) { right.x /= rlen; right.y /= rlen; right.z /= rlen; }

		float sx = sqrtf(
			pMonster->m_xmf4x4ToParent._11 * pMonster->m_xmf4x4ToParent._11 +
			pMonster->m_xmf4x4ToParent._12 * pMonster->m_xmf4x4ToParent._12 +
			pMonster->m_xmf4x4ToParent._13 * pMonster->m_xmf4x4ToParent._13);
		float sy = sqrtf(
			pMonster->m_xmf4x4ToParent._21 * pMonster->m_xmf4x4ToParent._21 +
			pMonster->m_xmf4x4ToParent._22 * pMonster->m_xmf4x4ToParent._22 +
			pMonster->m_xmf4x4ToParent._23 * pMonster->m_xmf4x4ToParent._23);
		float sz = sqrtf(
			pMonster->m_xmf4x4ToParent._31 * pMonster->m_xmf4x4ToParent._31 +
			pMonster->m_xmf4x4ToParent._32 * pMonster->m_xmf4x4ToParent._32 +
			pMonster->m_xmf4x4ToParent._33 * pMonster->m_xmf4x4ToParent._33);

		if (sx < 0.001f) sx = 1.0f;
		if (sy < 0.001f) sy = 1.0f;
		if (sz < 0.001f) sz = 1.0f;

		pMonster->m_xmf4x4ToParent._11 = right.x;
		pMonster->m_xmf4x4ToParent._12 = right.y;
		pMonster->m_xmf4x4ToParent._13 = right.z;

		pMonster->m_xmf4x4ToParent._21 = up.x;
		pMonster->m_xmf4x4ToParent._22 = up.y;
		pMonster->m_xmf4x4ToParent._23 = up.z;

		pMonster->m_xmf4x4ToParent._31 = look.x;
		pMonster->m_xmf4x4ToParent._32 = look.y;
		pMonster->m_xmf4x4ToParent._33 = look.z;
	}
	UpdateMonsterState(pMonster, state);
}

void CGameFramework::OnBossSpawned(long long bossID, const XMFLOAT3& pos, int hp, int maxHp)
{
	CScene* scene = m_ppScenes[m_nCurrentScene];
	if (!scene || !scene->m_pBoss) return;

	scene->m_pBoss->SetMaxHP((float)maxHp);
	scene->m_pBoss->SetHP((float)hp);         // ResetHP() 대신 서버가 준 실제 HP로 설정
	scene->m_pBoss->SetPosition(pos.x, pos.y, pos.z);
	scene->m_pBoss->TransitionTo(BossState::Idle);

	cout << "[BOSS] Spawned ID=" << bossID << " HP=" << hp << "/" << maxHp << "\n";
}
