#pragma once

#define FRAME_BUFFER_WIDTH 1920
#define FRAME_BUFFER_HEIGHT 1080

#include "Common.h"
#include "Timer.h"
#include "Player.h"
#include "Scene.h"
#include "CMonster.h"
#include "SoundManager.h"
#include <fmod.hpp>
#include <mutex>

enum class EPlayerModelType
{
	Knight,
	Wizard,
	Thief
};

class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	void CreateRtvAndDsvDescriptorHeaps();

	void CreateRenderTargetViews();
	void CreateDepthStencilView();

	void ChangeSwapChainState();

	void BuildObjects();
	void LoadSoundResources();
	void ReleaseObjects();

	void ProcessInput();
	void AnimateObjects();
	void FrameAdvance();

	void WaitForGpuComplete();
	void MoveToNextFrame();
	void MoveToNextScene(int i);

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	void SetSelectedPlayerModel(EPlayerModelType type) { m_eSelectedPlayerModel = type; }
	EPlayerModelType GetSelectedPlayerModel() const { return m_eSelectedPlayerModel; }

	// 보스를 처치할 때까지 걸린 시간(초). 게임 씬이 사라지기 전에 저장해두었다가
	// EndScene에서 "time: mm:ss" 형태로 표시하는 데 사용한다.
	void SetClearTime(float fSeconds) { m_fClearTime = fSeconds; }
	float GetClearTime() const { return m_fClearTime; }

	uint8_t CGameFramework::GetSelectedJob() const {
		cout << "[DEBUG] SelectedModel=" << static_cast<int>(m_eSelectedPlayerModel) << endl;

		switch (m_eSelectedPlayerModel) {
		case EPlayerModelType::Knight:return 0;
		case EPlayerModelType::Wizard:return 1;
		case EPlayerModelType::Thief:return 2;
		default: return 0;
		}
	}


	//long long FindNearestItemInRange(float range, XMFLOAT3 playerPos);
	//void CheckNearbyItemPrompt();
	//void ItemToHand(Item* pItem);
	//void ItemDropFromHand(Item* pItem);

	void RequestMoveToScene(int i) { m_nPendingScene = i; }

	void UpdatePlayerHP(float hp) {
		if (m_pPlayer->currentHP > hp) {
			CSoundManager::GetInstance()->PlaySFX("player_hurt");
			m_pCamera->StartShake(0.5f, 1.0f, 25.0f);
		}
		m_pPlayer->currentHP = hp;
		m_pPlayer->maxHP = 100.0f;
	}
	void UpdatePlayerGold(int gold) {
		m_pPlayer->Pgold = gold;
	}
	// The receive thread queues the spawn; the main thread applies it.
	void UpdateMyPlayerPosition(const XMFLOAT3& position, uint8_t job);

	void OnMonsterSpawned(int monsterID, const XMFLOAT3& pos, int state);
	void OnBossSpawned(long long bossID, const XMFLOAT3& pos, int hp, int maxHp);
	void UpdateMonsterState(CMonster* pMonster, int state);
	void UpdateMonsterPosition(int monsterID, const XMFLOAT3& pos, const XMFLOAT3& rot, int state);

	void ItemSpawned(long long itemID, const XMFLOAT3& pos, int type, int price);
	void UpdateItemPosition(long long itemID, const XMFLOAT3& pos);
	void UpdateItemRotation(long long itemID, const XMFLOAT3& look, const XMFLOAT3& right);

	void OnOtherClientConnected()
	{
		if (!m_ppScenes || !m_ppScenes[m_nCurrentScene]) return;
		if (isLoading || isStartScene) return;
		m_ppScenes[m_nCurrentScene]->OnOtherClientConnedted();
	}
	void UpdateOtherPlayerPosition(int clinetnum, XMFLOAT3 position)
	{
		m_ppScenes[m_nCurrentScene]->UpdateOtherPlayerPosition(clinetnum, position);
	}
	void UpdateOtherPlayerLook(int clientnum, XMFLOAT3 look, XMFLOAT3 right)
	{
		m_ppScenes[m_nCurrentScene]->UpdateOtherPlayerLook(clientnum, look, right);
	}
	void UpdateOtherPlayerAnimation(int clinetnum, int animNum)
	{
		m_ppScenes[m_nCurrentScene]->UpdateOtherPlayerAnimation(clinetnum, animNum);
	}

	const float Recognized_Range = 2.0f;

	void UpdateOtherPlayerRotate(int clinetnum, XMFLOAT3 right, XMFLOAT3 look)
	{
		m_ppScenes[m_nCurrentScene]->UpdateOtherPlayerRotate(clinetnum, right, look);
	}
	void UpdateOtherPlayerHP(int clientnum, float hp)
	{
		m_ppScenes[m_nCurrentScene]->UpdateOtherPlayerHP(clientnum, hp);
	}


	/*void UpdateItemPosition(long long id, const XMFLOAT3& position)
	{
		if (m_ppScenes && m_ppScenes[m_nCurrentScene]) {
			m_ppScenes[m_nCurrentScene]->UpdateItemPosition(id, position);
		}
	}*/

	CScene* GetCurrentScene() {
		if (!m_ppScenes) return nullptr;
		if (m_nCurrentScene < 0 || m_nCurrentScene >= m_nScenes) return nullptr;
		return m_ppScenes[m_nCurrentScene];
	}

	CCamera* GetCamera() { return m_pCamera; }

	bool isLoading = false;
	bool isStartScene = true;

private:
	void ApplyPendingMyPlayerPosition();

	std::mutex m_myPlayerPositionMutex;
	XMFLOAT3 m_pendingMyPlayerPosition = { 0.0f, 0.0f, 0.0f };
	uint8_t m_pendingMyPlayerJob = 0;
	bool m_hasPendingMyPlayerPosition = false;
	bool m_isServerSpawnApplied = false;

	HINSTANCE m_hInstance;
	HWND m_hWnd;

	int m_nWndClientWidth;
	int m_nWndClientHeight;

	IDXGIFactory4* m_pdxgiFactory = NULL;
	IDXGISwapChain3* m_pdxgiSwapChain = NULL;
	ID3D12Device* m_pd3dDevice = NULL;

	bool m_bMsaa4xEnable = false;
	UINT m_nMsaa4xQualityLevels = 0;

	static const UINT m_nSwapChainBuffers = 2;
	UINT m_nSwapChainBufferIndex;

	ID3D12Resource* m_ppd3dSwapChainBackBuffers[m_nSwapChainBuffers];
	ID3D12DescriptorHeap* m_pd3dRtvDescriptorHeap = NULL;

	ID3D12Resource* m_pd3dDepthStencilBuffer = NULL;
	ID3D12DescriptorHeap* m_pd3dDsvDescriptorHeap = NULL;

	ID3D12CommandAllocator* m_pd3dCommandAllocator = NULL;
	ID3D12CommandQueue* m_pd3dCommandQueue = NULL;
	ID3D12GraphicsCommandList* m_pd3dCommandList = NULL;

	ID3D12Fence* m_pd3dFence = NULL;
	UINT64 m_nFenceValues[m_nSwapChainBuffers];
	HANDLE m_hFenceEvent;

#if defined(_DEBUG)
	ID3D12Debug* m_pd3dDebugController;
#endif

	CGameTimer m_GameTimer;


	CScene* m_pScene = NULL;
	CPlayer* m_pPlayer = NULL;
	CCamera* m_pCamera = NULL;

	EPlayerModelType m_eSelectedPlayerModel = EPlayerModelType::Knight;
	float m_fClearTime = 0.0f; // 보스 처치까지 걸린 시간(초) - EndScene 표시용

	int m_nCurrentScene = 0;
	int m_nScene = 0;
	int m_nScenes = 0;
	CScene** m_ppScenes = NULL;

	int m_nPendingScene = -1;
	POINT m_ptOldCursorPos;

	// 마우스 휠(가운데) 버튼을 누르고 있는 동안에만 시점(카메라)을 회전시키기 위한 상태.
	// 평소엔 커서가 자유로워 기존 좌/우클릭(공격, 스킬강화 등)이 그대로 동작한다.
	bool m_bMouseOrbitDragging = false;
	POINT m_ptOrbitLastPos;

	bool m_bReloadInstancesRequested = false;
	bool m_bUsingTestSetter = false;

	_TCHAR						m_pszFrameRate[70];

	//server
	//float m_fLastPositionSendTime = 0.0f; 
};
