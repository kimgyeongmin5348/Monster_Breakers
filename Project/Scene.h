//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once

#include <array>
#include "Shader.h"
#include "Player.h"
#include "Object_Items.h"
#include "OtherPlayer.h"
#include "CMonster.h"
#include "CBossMonster.h"
#include "CAttackRangeEffect.h"
#include "Map.h"
#include "CollisionManager.h"
#include "CParticle.h"
#include "CText.h"
#include "Hpbar.h"
#include "CFireballSystem.h"
#include "CGreenSpiritSystem.h"
#include "CHitSparkSystem.h"
#include "CDeathBurstSystem.h"
#include "CWeaponThrowSystem.h"
#include "CBeamSystem.h"
#include "GroundCrackEffect.h"
#include "CSwordTrailEffect.h"

#define MAX_LIGHTS						16 

#define POINT_LIGHT						1
#define SPOT_LIGHT						2
#define DIRECTIONAL_LIGHT				3

struct LIGHT
{
	XMFLOAT4							m_xmf4Ambient;
	XMFLOAT4							m_xmf4Diffuse;
	XMFLOAT4							m_xmf4Specular;
	XMFLOAT3							m_xmf3Position;
	float 								m_fFalloff;
	XMFLOAT3							m_xmf3Direction;
	float 								m_fTheta; //cos(m_fTheta)
	XMFLOAT3							m_xmf3Attenuation;
	float								m_fPhi; //cos(m_fPhi)
	bool								m_bEnable;
	int									m_nType;
	float								m_fRange;
	float								padding;
};										
										
struct LIGHTS							
{										
	LIGHT								m_pLights[MAX_LIGHTS];
	XMFLOAT4							m_xmf4GlobalAmbient;
	int									m_nLights;
};

//ID3D12Resource* m_pd3dShadowMap = nullptr;
//D3D12_CPU_DESCRIPTOR_HANDLE m_d3dShadowDSV = {};
//D3D12_GPU_DESCRIPTOR_HANDLE m_d3dShadowSRV = {}; 
//
//ID3D12DescriptorHeap* m_pd3dShadowDsvHeap = nullptr; 

struct CB_SHADOW_INFO
{
	XMFLOAT4X4 m_xmf4x4LightView;
	XMFLOAT4X4 m_xmf4x4LightProj;
	float      m_fShadowBias;
	float      pad0[3];
	XMFLOAT2   m_xmf2ShadowTexel;
	float      pad1[2];
};

//ID3D12Resource* m_pd3dcbShadow = nullptr;
//CB_SHADOW_INFO* m_pcbMappedShadow = nullptr;
//
//CShadowShader* m_pShadowShader = nullptr;
//CSkinnedShadowShader* m_pSkinnedShadowShader = nullptr;
//
//D3D12_VIEWPORT m_ShadowViewport = {};
//D3D12_RECT     m_ShadowScissor = {};

struct MonsterDesc {
	const char* modelPath;
	int         startID;
	float       hp;
	float       scale;
};

static const MonsterDesc MONSTER_DESCS[] = {
	{ "Model/Monster/CactusPA.bin",       10001, 150.0f, 1.0f },
	{ "Model/Monster/BattleBeePA.bin",    10004, 100.0f, 1.0f },
	{ "Model/Monster/NagaWizardPA.bin",   10007, 100.0f, 1.0f },
	{ "Model/Monster/CyclopsPA.bin",      10010, 200.0f, 1.0f },
	{ "Model/Monster/BishopKnightPA.bin", 10013, 100.0f, 1.0f },
	{ "Model/Monster/SalamanderPA.bin",   10016, 120.0f, 1.0f },
	{ "Model/Monster/MushroomAngryPA.bin",10019, 100.0f, 1.0f },
	{ "Model/Monster/FishmanPA.bin",      10022, 100.0f, 1.0f },
	{ "Model/Monster/StingRayPA.bin",     10025, 100.0f, 1.0f },
};

class CScene
{
public:
    CScene();
    ~CScene();

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void BuildDefaultLightsAndMaterials(bool toggle);
	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseObjects();

	ID3D12RootSignature *CreateGraphicsRootSignature(ID3D12Device *pd3dDevice);
	ID3D12RootSignature *GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }

	bool ProcessInput(UCHAR *pKeysBuffer);
    virtual void AnimateObjects(float fTimeElapsed);
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera=NULL);
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv);

	void ReleaseUploadBuffers();

	void InitializeCollisionSystem();
	void GenerateGameObjectsBoundingBox();

	void BuildSimpleUI(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateUI(ID3D12GraphicsCommandList* pd3dCommandList);

	// Party HP UI: shows other party members' (Knight/Thief) HP top-right, wizard-only
	void CreatePartyHPUI(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void UpdatePartyHPUI();

	// Judging/debug overlay (top-left text, shown while m_bDebugMode is on - 'P' toggles it)
	void CreateDebugOverlay(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void UpdateDebugOverlay();

protected:
	// 실제 렌더 본문(공통). 자식들은 이걸 override하면 됨.
	virtual void RenderImpl(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

	// (선택) ShadowPass만 분리하고 싶으면
	virtual void RenderShadowPass(ID3D12GraphicsCommandList* pd3dCommandList);

protected:
	// 현재 프레임 백버퍼 핸들 보관(오버로드간 브릿지)
	D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentRTV = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentDSV = {};
	bool m_bHasCurrentRT = false;

private:
	std::vector<CTexture*> m_UITextures;

	void UpdatePartyHPBar(CTextureToScreenShader* pBar, CText* pLabel, float& fPrevWidth, float fyTop, OtherPlayer* pTarget);

public:
	CPlayer								*m_pPlayer = NULL;
	std::unordered_map<std::string, CTexture*> m_textureMap;

	std::vector<OtherPlayer*>				m_vPlayers;
	int										m_nOtherPlayers = 0;
	OtherPlayer**							m_ppOtherPlayers;

	void SetPlayer(CPlayer* pPlayer) { m_pPlayer = pPlayer; }
	CPlayer* GetPlayer() { return(m_pPlayer); }
 
	ID3D12RootSignature						*m_pd3dGraphicsRootSignature = NULL;
	ID3D12Device* Device = NULL;
	ID3D12GraphicsCommandList* Commandlist = NULL;

public:
	// ----- Shadow Mapping -----
	static const UINT SHADOW_MAP_SIZE = 2048;
	static const UINT SHADOW_SRV_INDEX = 999; // SRV heap (0~999 중 마지막 사용)

	ID3D12Resource* m_pd3dShadowMap = nullptr;
	ID3D12DescriptorHeap* m_pd3dShadowDsvHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dShadowDSV = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dShadowSRV = {};

	ID3D12Resource* m_pd3dcbShadow = nullptr;
	CB_SHADOW_INFO* m_pcbMappedShadow = nullptr;

	CShadowShader* m_pShadowShader = nullptr;
	CSkinnedShadowShader* m_pSkinnedShadowShader = nullptr;

	D3D12_VIEWPORT m_ShadowViewport = {};
	D3D12_RECT     m_ShadowScissor = {};

	bool m_bEnableShadow = false;

protected:
	//ID3D12RootSignature					*m_pd3dGraphicsRootSignature = NULL;

	static ID3D12DescriptorHeap			*m_pd3dCbvSrvDescriptorHeap;

	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dCbvCPUDescriptorStartHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dCbvGPUDescriptorStartHandle;
	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dSrvCPUDescriptorStartHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dSrvGPUDescriptorStartHandle;

	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dCbvCPUDescriptorNextHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dCbvGPUDescriptorNextHandle;
	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dSrvCPUDescriptorNextHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dSrvGPUDescriptorNextHandle;

	//server
	//std::unordered_map<long long, CRemotePlayer*> m_remotePlayers;
	//CRITICAL_SECTION m_csRemotePlayers;

public:
	static void CreateCbvSrvDescriptorHeaps(ID3D12Device *pd3dDevice, int nConstantBufferViews, int nShaderResourceViews);

	static D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferViews(ID3D12Device *pd3dDevice, int nConstantBufferViews, ID3D12Resource *pd3dConstantBuffers, UINT nStride);
	static void CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex);
	void CreateShadowResources(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorStartHandle() { return(m_d3dCbvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorStartHandle() { return(m_d3dCbvGPUDescriptorStartHandle); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorStartHandle() { return(m_d3dSrvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorStartHandle() { return(m_d3dSrvGPUDescriptorStartHandle); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorNextHandle() { return(m_d3dCbvCPUDescriptorNextHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorNextHandle() { return(m_d3dCbvGPUDescriptorNextHandle); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorNextHandle() { return(m_d3dSrvCPUDescriptorNextHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorNextHandle() { return(m_d3dSrvGPUDescriptorNextHandle); }

	float								m_fElapsedTime = 0.0f;

	std::vector<CGameObject*> m_GameObjects;  
	std::vector<CMonster*>	  m_Monsters;
	CBossMonster* m_pBoss = nullptr;
	CGroundAttackRangeEffect* m_pGroundAttackRangeEffect = nullptr;
	std::vector<CShader*>     m_Shaders;
	
	// 보스 사망->엔딩 씬 전환까지의 대기 시간 처리용
	static constexpr float BOSS_DEATH_TO_END_DELAY = 3.0f; // 보스 사망 후 엔딩 씬으로 넘어가기까지 대기 시간(초)
	float m_fBossDeathTimer = 0.0f;
	bool  m_bEndSceneRequested = false;

	CLoadedModelInfo* m_pModel = NULL; // 플레이어 모델
	CLoadedModelInfo* m_pKnightModel = NULL; // 기사
	CLoadedModelInfo* m_pWizardModel = NULL; // 법사
	CLoadedModelInfo* m_pThiefModel = NULL;  // 도적

	XMFLOAT3							m_xmf3RotatePosition = XMFLOAT3(0.0f, 0.0f, 0.0f);

	CSkyBox								*m_pSkyBox = NULL;
	CHeightMapTerrain					*m_pTerrain = NULL;
	Map									*m_pMap = NULL;

	LIGHT								*m_pLights = NULL;
	int									m_nLights = 0;

	XMFLOAT4							m_xmf4GlobalAmbient;

	ID3D12Resource						*m_pd3dcbLights = NULL;
	LIGHTS								*m_pcbMappedLights = NULL;

	CCollisionManager					m_CollisionManager;
	bool								m_bDebugMode = false;

	static constexpr int DEBUG_TEXT_LINES = 5;
	CText* m_pDebugTexts[DEBUG_TEXT_LINES] = { nullptr };
	int    m_nCurrentFps = 0;
	int    m_nFpsFrameCount = 0;
	float  m_fFpsAccumTime = 0.0f;

	CFireballSystem* m_pFireballSystem = nullptr;
	CGreenSpiritSystem* m_pGreenSpiritSystem = nullptr;
	CHitSparkSystem* m_pHitSparkSystem = nullptr;
	CDeathBurstSystem* m_pDeathBurstSystem = nullptr;
	CWeaponThrowSystem* m_pWeaponThrowSystem = nullptr;
	CBeamSystem* m_pBeamSystem = nullptr;
	CGroundCrackEffect* m_pGroundCrackEffect = nullptr;
	CSwordTrailEffect* m_pSwordTrailEffect = nullptr;

	// npc ui
	CText* m_pMissionText = nullptr;   // 미션 설명 텍스트
	CText* m_pMissionGoldText = nullptr;
	CText* m_pMissionProgressText = nullptr;
	CTextureToScreenShader* m_pMissionBgShader = nullptr; // 미션 배경 ui
	bool        m_bMissionUIVisible = false;

	// Party HP UI (other party members' HP shown top-right, wizard-only)
	static constexpr float PARTY_HP_LEFT = 0.60f;
	static constexpr float PARTY_HP_WIDTH = 0.35f;
	static constexpr float PARTY_HP_HEIGHT = 0.08f;
	static constexpr float PARTY_HP_GAP = 0.03f;
	static constexpr float PARTY_HP_TOP = 0.0f; // top edge of the party HP block sits at vertical screen center

	CTextureToScreenShader* m_pKnightPartyHPBarBg = nullptr; // black backdrop showing HP lost
	CTextureToScreenShader* m_pThiefPartyHPBarBg = nullptr;
	CTextureToScreenShader* m_pKnightPartyHPBar = nullptr;
	CTextureToScreenShader* m_pThiefPartyHPBar = nullptr;
	CText* m_pKnightPartyLabel = nullptr;
	CText* m_pThiefPartyLabel = nullptr;
	float m_fKnightPartyHPBarPrevWidth = -1.0f;
	float m_fThiefPartyHPBarPrevWidth = -1.0f;

	void ShowMissionText(const std::wstring& text);
	void ShowMissionProgress(int currentCount, int targetCount);
	void HideMissionProgress();
	void HideMissionText();

	// skill cooltime
	static constexpr int   SKILL_COUNT = 3;
	static constexpr float SKILL_BASE_CD[3] = { 10.0f, 10.0f, 10.0f }; // 스킬별 기본 쿨타임(초)

	float  m_fSkillCooldown[SKILL_COUNT] = {};   // 남은 쿨타임(초)
	float  m_fSkillMaxCooldown[SKILL_COUNT] = {};   // 현재 레벨 기준 최대 쿨타임

	// 쿨타임 텍스트 (남은 초 표시)
	std::array<CText*, SKILL_COUNT> m_pCooldownTexts = {};

	// 외부(플레이어 입력)에서 쿨타임 발동
	void TriggerSkillCooldown(int skillIndex);

	// 쿨타임 계산
	float CalcMaxCooldown(int skillIndex) const;
	// 쿨타임 중인지 확인
	bool IsSkillOnCooldown(int skillIndex) const
	{
		if (skillIndex < 0 || skillIndex >= SKILL_COUNT) return false;
		return m_fSkillCooldown[skillIndex] > 0.0f;
	}

	POINT m_ptPos;

public:

	//server
	
	void OnOtherClientConnedted()
	{
		if (!m_ppOtherPlayers) return;
		for (int i = 0; i < m_nOtherPlayers; ++i)
		{
			m_ppOtherPlayers[i]->isConnedted = true;
		}
	}

	void UpdateOtherPlayerPosition(int clientnum, XMFLOAT3 position)
	{
		if (!m_ppOtherPlayers) return;
		if (clientnum < 0 || clientnum >= m_nOtherPlayers) return;
		if (!m_ppOtherPlayers[clientnum]) return;
		m_ppOtherPlayers[clientnum]->SetPosition(position);
	}
	void UpdateOtherPlayerLook(int clientnum, XMFLOAT3 look, XMFLOAT3 right)
	{
		if (!m_ppOtherPlayers) return;
		if (clientnum < 0 || clientnum >= m_nOtherPlayers) return;
		if (!m_ppOtherPlayers[clientnum]) return; 
		m_ppOtherPlayers[clientnum]->Rotate(look, right);
	}
	void UpdateOtherPlayerAnimation(int clientnum, int animNum)
	{
		if (!m_ppOtherPlayers) return;
		if (clientnum < 0 || clientnum >= m_nOtherPlayers) return;
		if (!m_ppOtherPlayers[clientnum]) return; 
		m_ppOtherPlayers[clientnum]->targetAnim = animNum;
	}
	void UpdateOtherPlayerRotate(int clientnum, XMFLOAT3 right, XMFLOAT3 look)
	{
		if (!m_ppOtherPlayers) return;
		if (clientnum < 0 || clientnum >= m_nOtherPlayers) return;
		if (!m_ppOtherPlayers[clientnum]) return; 
		m_ppOtherPlayers[clientnum]->m_xmf3Look = look;
		m_ppOtherPlayers[clientnum]->m_xmf3Right = right;
	}
	void UpdateOtherPlayerHP(int clientnum, float hp)
	{
		if (!m_ppOtherPlayers) return;
		if (clientnum < 0 || clientnum >= m_nOtherPlayers) return;
		if (!m_ppOtherPlayers[clientnum]) return; 
		m_ppOtherPlayers[clientnum]->currentHP = hp;
		m_ppOtherPlayers[clientnum]->maxHP += hp;
	}
};

enum class InputStep { EnterID, EnterIP, Done };

class CStartScene : public CScene
{
public:
	CStartScene(){}
	~CStartScene(){}

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseObjects();

	virtual void RenderImpl(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual void AnimateObjects(float fTimeElapsed);

	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

private:
	std::vector<CGameObject*> m_GameObjects;

	InputStep m_inputStep = InputStep::EnterID;
	std::string m_inputID;
	std::string m_inputIP;
	bool m_networkInitialized = false;
	bool m_textDirty = false;
	CText* m_pFontID = nullptr;
	CText* m_pFontIP = nullptr;
};

class CSelectScene : public CScene
{
public:
	CSelectScene(){}
	~CSelectScene(){}

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseObjects();

	virtual void RenderImpl(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual void AnimateObjects(float fTimeElapsed);

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

private:
	bool loading = false;
	bool m_bLoadingRenderedOnce = false;
	int m_SceneId = -1;
};

class CEndScene : public CScene
{
public:
	CEndScene(){}
	~CEndScene(){}

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseObjects();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual void UpdateUI(ID3D12GraphicsCommandList* pd3dCommandList) override;

private:
	CText* m_pIDText = nullptr;   // "ID: <플레이어 이름>"
	CText* m_pTimeText = nullptr; // "time: mm:ss" (보스 처치까지 걸린 시간)
};