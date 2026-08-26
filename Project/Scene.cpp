//-----------------------------------------------------------------------------
// File: CScene.cpp
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "Scene.h"
#include "Network.h"
#include "GameFramework.h"
#include "SoundManager.h"

#include <random>
#include <array>
#include <unordered_map>

static std::mt19937 rng{ std::random_device{}() };
static bool Chance(int percent) { // 0~100
	std::uniform_int_distribution<int> dist(1, 100);
	return dist(rng) <= percent;
}

// 몬스터 종류별 정보: { 모델 파일명, 서버 시작ID, HP, 스케일 }
//struct MonsterDesc {
//	const char* modelPath;
//	int         startID;
//	float       hp;
//	float       scale;
//};
//
//static const MonsterDesc MONSTER_DESCS[] = {
//	{ "Model/Monster/FishmanPA.bin",      10001, 100.0f, 1.0f },
//	{ "Model/Monster/CactusPA.bin",   10004, 150.0f, 1.0f },
//	{ "Model/Monster/BattleBeePA.bin",         10007, 100.0f, 1.0f },
//	{ "Model/Monster/CyclopsPA.bin",        10010, 200.0f, 1.0f },
//	{ "Model/Monster/BishopKnightPA.bin",        10013, 100.0f, 1.0f },
//	{ "Model/Monster/NagaWizardPA.bin",  10016, 100.0f, 1.0f },
//	{ "Model/Monster/SalamanderPA.bin",     10019, 120.0f, 1.0f },
//	{ "Model/Monster/MushroomAngryPA.bin",     10022, 100.0f, 1.0f },
//	{ "Model/Monster/StingRayPA.bin",       10025, 100.0f, 1.0f },
//};

extern CGameFramework gGameFramework;

ID3D12DescriptorHeap* CScene::m_pd3dCbvSrvDescriptorHeap = NULL;

D3D12_CPU_DESCRIPTOR_HANDLE	CScene::m_d3dCbvCPUDescriptorStartHandle;
D3D12_GPU_DESCRIPTOR_HANDLE	CScene::m_d3dCbvGPUDescriptorStartHandle;
D3D12_CPU_DESCRIPTOR_HANDLE	CScene::m_d3dSrvCPUDescriptorStartHandle;
D3D12_GPU_DESCRIPTOR_HANDLE	CScene::m_d3dSrvGPUDescriptorStartHandle;

D3D12_CPU_DESCRIPTOR_HANDLE	CScene::m_d3dCbvCPUDescriptorNextHandle;
D3D12_GPU_DESCRIPTOR_HANDLE	CScene::m_d3dCbvGPUDescriptorNextHandle;
D3D12_CPU_DESCRIPTOR_HANDLE	CScene::m_d3dSrvCPUDescriptorNextHandle;
D3D12_GPU_DESCRIPTOR_HANDLE	CScene::m_d3dSrvGPUDescriptorNextHandle;

CScene::CScene()
{}

CScene::~CScene()
{}

void CScene::BuildDefaultLightsAndMaterials(bool toggle)
{
	m_nLights = 1;
	m_pLights = new LIGHT[m_nLights];
	::ZeroMemory(m_pLights, sizeof(LIGHT) * m_nLights);

	//m_xmf4GlobalAmbient = XMFLOAT4(0.08f, 0.08f, 0.08f, 1.0f);
	m_xmf4GlobalAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);

	//m_pLights[0].m_bEnable = true;
	//m_pLights[0].m_nType = POINT_LIGHT;
	//m_pLights[0].m_fRange = 17.0f;
	//m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	//m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	//m_pLights[0].m_xmf4Specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	//m_pLights[0].m_xmf3Position = XMFLOAT3(3.0f, 5.0f, 30.0f);
	//m_pLights[0].m_xmf3Attenuation = XMFLOAT3(0.5f, 0.05f, 0.0001f);

	m_pLights[0].m_bEnable = toggle;
	m_pLights[0].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_pLights[0].m_xmf4Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	XMFLOAT3 lightDir = XMFLOAT3(0.5f, -1.0f, 0.5f); // 약간 대각선 아래로
	XMVECTOR vLightDir = XMLoadFloat3(&lightDir);
	vLightDir = XMVector3Normalize(vLightDir);
	XMStoreFloat3(&m_pLights[0].m_xmf3Direction, vLightDir);
	m_pLights[0].m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_pLights[0].m_fRange = 0.0f;
	m_pLights[0].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_pLights[0].m_fFalloff = 0.0f;
	m_pLights[0].m_fPhi = 0.0f;
	m_pLights[0].m_fTheta = 0.0f;

	//m_pLights[2].m_bEnable = true;
	//m_pLights[2].m_nType = SPOT_LIGHT;
	//m_pLights[2].m_fRange = 600.0f;
	//m_pLights[2].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	//m_pLights[2].m_xmf4Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	//m_pLights[2].m_xmf4Specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.0f);
	//m_pLights[2].m_xmf3Position = XMFLOAT3(-43.0f, 10.0f, -45.0f);
	//m_pLights[2].m_xmf3Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	//m_pLights[2].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	//m_pLights[2].m_fFalloff = 8.0f;
	//m_pLights[2].m_fPhi = (float)cos(XMConvertToRadians(90.0f));
	//m_pLights[2].m_fTheta = (float)cos(XMConvertToRadians(30.0f));

	//m_pLights[3].m_bEnable = true;
	//m_pLights[3].m_nType = SPOT_LIGHT;
	//m_pLights[3].m_fRange = 600.0f;
	//m_pLights[3].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	//m_pLights[3].m_xmf4Diffuse = XMFLOAT4(0.3f, 0.7f, 0.0f, 1.0f);
	//m_pLights[3].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	//m_pLights[3].m_xmf3Position = XMFLOAT3(45.0f, 10.0f, -28.0f);
	//m_pLights[3].m_xmf3Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	//m_pLights[3].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	//m_pLights[3].m_fFalloff = 8.0f;
	//m_pLights[3].m_fPhi = (float)cos(XMConvertToRadians(90.0f));
	//m_pLights[3].m_fTheta = (float)cos(XMConvertToRadians(30.0f));

	//m_pLights[4].m_bEnable = true;
	//m_pLights[4].m_nType = SPOT_LIGHT;
	//m_pLights[4].m_fRange = 600.0f;
	//m_pLights[4].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	//m_pLights[4].m_xmf4Diffuse = XMFLOAT4(0.8f, 0.2f, 0.2f, 1.0f);
	//m_pLights[4].m_xmf4Specular = XMFLOAT4(0.6f, 0.2f, 0.2f, 0.0f);
	//m_pLights[4].m_xmf3Position = XMFLOAT3(-63.0f, 10.0f, -14.0f);
	//m_pLights[4].m_xmf3Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	//m_pLights[4].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	//m_pLights[4].m_fFalloff = 8.0f;
	//m_pLights[4].m_fPhi = (float)cos(XMConvertToRadians(90.0f));
	//m_pLights[4].m_fTheta = (float)cos(XMConvertToRadians(30.0f));
}

void CScene::InitializeCollisionSystem()
{
	BoundingBox worldBounds(XMFLOAT3(140.0f, 0.f, 30.0f), XMFLOAT3(200.0f, 100.0f, 150.0f));
	m_CollisionManager.Build(worldBounds, 100, 4);

	for (auto* obj : m_GameObjects) {
		m_CollisionManager.InsertObject(obj);
	}

	m_CollisionManager.SetFireballSystem(m_pFireballSystem);
	m_CollisionManager.SetWeaponThrowSystem(m_pWeaponThrowSystem);
	m_CollisionManager.SetHitSparkSystem(m_pHitSparkSystem);

	for (auto* obj : m_Monsters) {
		m_CollisionManager.SetMonsters(&m_Monsters);
	}
	m_CollisionManager.SetBoss(m_pBoss); // 보스도 CollisionManager가 알도록 등록

	for (const auto& pair : m_pMap->m_mInstanceGroups)
	{
		int modelIndex = pair.first;
		const InstanceGroup& group = pair.second;

		if (!group.pModel) continue;

		std::string strFrameName = group.pModel->GetFrameName();

		if (std::string::npos != strFrameName.find("hill") ||
			std::string::npos != strFrameName.find("grass") ||
			std::string::npos != strFrameName.find("sand") ||
			std::string::npos != strFrameName.find("trail") ||
			std::string::npos != strFrameName.find("bush") ||
			std::string::npos != strFrameName.find("plank") ||
			std::string::npos != strFrameName.find("banana") ||
			std::string::npos != strFrameName.find("coin") ||
			std::string::npos != strFrameName.find("apple") ||
			std::string::npos != strFrameName.find("melon") ||
			std::string::npos != strFrameName.find("carrot") ||
			std::string::npos != strFrameName.find("onion") ||
			std::string::npos != strFrameName.find("potato") ||
			std::string::npos != strFrameName.find("Plane") ||
			std::string::npos != strFrameName.find("bridge") ||
			0 == strFrameName.find("stone_0") ||
			std::string::npos != strFrameName.find("landscape") ||
			std::string::npos != strFrameName.find("mountains") ||
			std::string::npos != strFrameName.find("dirt") ||
			std::string::npos != strFrameName.find("log") ||
			std::string::npos != strFrameName.find("swordfish") ||
			std::string::npos != strFrameName.find("fire") ||
			std::string::npos != strFrameName.find("armor") ||
			std::string::npos != strFrameName.find("arrow") ||
			std::string::npos != strFrameName.find("axe") ||
			std::string::npos != strFrameName.find("blacksmith_hammer") ||
			std::string::npos != strFrameName.find("crossbow") ||
			std::string::npos != strFrameName.find("dagger") ||
			std::string::npos != strFrameName.find("long_bow") ||
			std::string::npos != strFrameName.find("short_bow") ||
			std::string::npos != strFrameName.find("mace") ||
			std::string::npos != strFrameName.find("one_handed_sword") ||
			std::string::npos != strFrameName.find("two_handed_sword") ||
			std::string::npos != strFrameName.find("shield_02") ||
			std::string::npos != strFrameName.find("one_handed_hammer") ||
			std::string::npos != strFrameName.find("two_handed_hammer") ||
			std::string::npos != strFrameName.find("rune_stone_small") ||
			std::string::npos != strFrameName.find("helmet")
			)
		{
			continue;
		}

		// 캐시된 월드 바운딩 박스 배열을 순회하며 쿼드트리에 밀어 넣기
		for (size_t i = 0; i < group.vWorldColliders.size(); ++i)
		{
			ColliderInfo collider = group.vWorldColliders[i];
			collider.pOwner = nullptr;
			m_CollisionManager.InsertCollider(collider);
		}

	}

	//m_CollisionManager.PrintTree();
}

void CScene::GenerateGameObjectsBoundingBox()
{
	m_pPlayer->CalculateBoundingBox();

	for (auto* obj : m_GameObjects) {
		obj->CalculateBoundingBox();
	}

	for (auto* obj : m_Monsters) {
		obj->CalculateBoundingBox();
	}

	if (m_pBoss) m_pBoss->CalculateBoundingBox();

	if (m_pMap) {
		m_pMap->BuildWorldBoundingBoxes();
	}
}

void CScene::BuildSimpleUI(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	struct UIInfo { std::wstring path; float left; float top; float width; float height; };

	static const std::unordered_map<CLoadedModelInfo*, std::array<const wchar_t*, 3>> skillImageMap = {
		{ m_pKnightModel, { L"Image/방패막기.dds", L"Image/강타.dds",    L"Image/도발.dds"   } },
		{ m_pWizardModel, { L"Image/화염구.dds",   L"Image/공격력버프.dds",  L"Image/체력버프.dds" } },
		{ m_pThiefModel,  { L"Image/던지기.dds",   L"Image/휘두르기.dds", L"Image/뒤로순보.dds" } },
	};

	std::array<float, 3> skillSlotX{ 0.25f, 0.50f, 0.75f };

	std::vector<UIInfo> uiList;

	uiList.push_back({ L"Image/hpbar.dds", 0.15f, 0.7f, 0.9f, 0.2f });
	uiList.push_back({ L"Image/mission.dds", -0.95f, 0.8f, -0.5f, 0.25f });

	auto it = skillImageMap.find(m_pModel);
	if (it != skillImageMap.end()) {
		for (int i = 0; i < 3; ++i)
			uiList.push_back({ it->second[i], skillSlotX[i], 0.2f, -0.4f, 0.4f });
	}
	for (size_t i = 0; i < uiList.size(); ++i)
	{
		// 텍스처 생성 및 로드
		CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
		pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, const_cast<wchar_t*>(uiList[i].path.c_str()), RESOURCE_TEXTURE2D, 0);

		CScene::CreateShaderResourceViews(pd3dDevice, pTexture, 0, 15);

		CTextureToScreenShader* pShader = new CTextureToScreenShader(1);
		pShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

		CScreenRectMeshTextured* pMesh = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, uiList[i].left, uiList[i].top, uiList[i].width, uiList[i].height);
		pShader->SetMesh(0, pMesh);
		pShader->SetTexture(pTexture);

		// mission.dds는 NPC 근접 상호작용 시에만 보이도록 기본 비활성화
		bool bIsMissionBg = (uiList[i].path == L"Image/mission.dds");
		pShader->SetVisible(!bIsMissionBg);
		if (bIsMissionBg)
			m_pMissionBgShader = pShader;

		m_Shaders.push_back(pShader);

		m_UITextures.push_back(pTexture);
	}

	std::array<float, 3> cooldownSlotX{ 0.25f, 0.50f, 0.75f };

	for (int i = 0; i < SKILL_COUNT; ++i)
	{
		// 최대 쿨타임 초기화 (레벨 반영)
		m_fSkillMaxCooldown[i] = CalcMaxCooldown(i);
		m_fSkillCooldown[i] = 0.0f;

		CText* pText = new CText(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, L"", 0.3f + i * 0.25f, -0.6f);
		pText->SetVisible(false);
		m_pCooldownTexts[i] = pText;
		m_GameObjects.push_back(pText);
	}

	CreatePartyHPUI(pd3dDevice, pd3dCommandList);
	CreateDebugOverlay(pd3dDevice, pd3dCommandList);
}

void CScene::CreateDebugOverlay(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	const float x = -0.95f;
	const float topY = 0.65f;
	const float lineGap = 0.09f;

	for (int i = 0; i < DEBUG_TEXT_LINES; ++i)
	{
		CText* pText = new CText(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, L"", x, topY - lineGap * i);
		pText->SetVisible(false);
		m_pDebugTexts[i] = pText;
		m_GameObjects.push_back(pText);
	}
}

void CScene::CreatePartyHPUI(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	auto createBar = [&](const wchar_t* pTexturePath, float fyTop) -> CTextureToScreenShader*
	{
		CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
		pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, const_cast<wchar_t*>(pTexturePath), RESOURCE_TEXTURE2D, 0);
		CScene::CreateShaderResourceViews(pd3dDevice, pTexture, 0, 15);

		CTextureToScreenShader* pShader = new CScreenShader(1);
		pShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

		CScreenRectMeshTextured* pMesh = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, PARTY_HP_LEFT, PARTY_HP_WIDTH, fyTop, PARTY_HP_HEIGHT);
		pShader->SetMesh(0, pMesh);
		pShader->SetTexture(pTexture);
		pShader->SetVisible(false); // hidden by default until a connected party member of that job is found

		m_Shaders.push_back(pShader);
		m_UITextures.push_back(pTexture);
		return pShader;
	};

	auto createLabel = [&](const std::wstring& text, float fyTop) -> CText*
	{
		CText* pLabel = new CText(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, text, PARTY_HP_LEFT, fyTop + 0.09f);
		pLabel->SetVisible(false);
		m_GameObjects.push_back(pLabel);
		return pLabel;
	};

	const float fKnightTop = PARTY_HP_TOP;
	const float fThiefTop = PARTY_HP_TOP - (PARTY_HP_HEIGHT + PARTY_HP_GAP);

	// Black backdrop is drawn first at full width so the drained portion of HP stays visible as black
	// once the foreground bar (drawn right after, same rect) shrinks.
	m_pKnightPartyHPBarBg = createBar(L"Image/black.dds", fKnightTop);
	m_pKnightPartyHPBar = createBar(L"Image/hp.dds", fKnightTop);
	m_pKnightPartyLabel = createLabel(L"Knight", fKnightTop);

	m_pThiefPartyHPBarBg = createBar(L"Image/black.dds", fThiefTop);
	m_pThiefPartyHPBar = createBar(L"Image/hp.dds", fThiefTop);
	m_pThiefPartyLabel = createLabel(L"Thief", fThiefTop);
}

void CScene::UpdatePartyHPBar(CTextureToScreenShader* pBar, CText* pLabel, float& fPrevWidth, float fyTop, OtherPlayer* pTarget)
{
	if (!pBar) return;

	bool bVisible = (pTarget != nullptr);
	pBar->SetVisible(bVisible);
	if (pLabel) pLabel->SetVisible(bVisible);
	if (!bVisible) return;

	float hpRatio = (pTarget->maxHP > 0.0f) ? (pTarget->currentHP / pTarget->maxHP) : 0.0f;
	if (hpRatio < 0.0f) hpRatio = 0.0f;
	if (hpRatio > 1.0f) hpRatio = 1.0f;

	float newWidth = hpRatio * PARTY_HP_WIDTH;
	if (fabs(fPrevWidth - newWidth) > 0.01f && pBar->m_nMeshes > 0 && pBar->m_ppMeshes[0])
	{
		fPrevWidth = newWidth;
		auto* pRect = static_cast<CScreenRectMeshTextured*>(pBar->m_ppMeshes[0]);
		pRect->UpdateRect(PARTY_HP_LEFT, newWidth, fyTop, PARTY_HP_HEIGHT);
	}
}

void CScene::UpdatePartyHPUI()
{
	// Party HP UI is only shown to a local wizard player.
	bool bIsWizard = (m_pModel != nullptr && m_pModel == m_pWizardModel);

	OtherPlayer* pKnight = nullptr;
	if (bIsWizard && m_ppOtherPlayers && m_nOtherPlayers >= 2)
	{
		if (m_ppOtherPlayers[0] && m_ppOtherPlayers[0]->isConnedted) pKnight = m_ppOtherPlayers[0];
		else if (m_ppOtherPlayers[1] && m_ppOtherPlayers[1]->isConnedted) pKnight = m_ppOtherPlayers[1];
	}

	OtherPlayer* pThief = nullptr;
	if (bIsWizard && m_ppOtherPlayers && m_nOtherPlayers >= 6)
	{
		if (m_ppOtherPlayers[4] && m_ppOtherPlayers[4]->isConnedted) pThief = m_ppOtherPlayers[4];
		else if (m_ppOtherPlayers[5] && m_ppOtherPlayers[5]->isConnedted) pThief = m_ppOtherPlayers[5];
	}

	if (m_pKnightPartyHPBarBg) m_pKnightPartyHPBarBg->SetVisible(bIsWizard && pKnight != nullptr);
	if (m_pThiefPartyHPBarBg) m_pThiefPartyHPBarBg->SetVisible(bIsWizard && pThief != nullptr);

	UpdatePartyHPBar(m_pKnightPartyHPBar, m_pKnightPartyLabel, m_fKnightPartyHPBarPrevWidth, PARTY_HP_TOP, pKnight);
	UpdatePartyHPBar(m_pThiefPartyHPBar, m_pThiefPartyLabel, m_fThiefPartyHPBarPrevWidth, PARTY_HP_TOP - (PARTY_HP_HEIGHT + PARTY_HP_GAP), pThief);
}

void CScene::UpdateUI(ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (auto* obj : m_GameObjects) {
		if (auto* textObj = dynamic_cast<CText*>(obj)) {
			// 쿨타임 텍스트 / 미션 텍스트는 각자 별도 로직으로 갱신되므로
			// 여기서 레벨 텍스트("LV. n")로 덮어쓰면 안 됨
			bool bIsCooldownText = false;
			for (int i = 0; i < SKILL_COUNT; ++i)
				if (m_pCooldownTexts[i] == textObj) { bIsCooldownText = true; break; }

			bool bIsDebugText = false;
			for (int i = 0; i < DEBUG_TEXT_LINES; ++i)
				if (m_pDebugTexts[i] == textObj) { bIsDebugText = true; break; }

			if (bIsCooldownText || bIsDebugText || textObj == m_pMissionText || textObj == m_pMissionProgressText ||
				textObj == m_pKnightPartyLabel || textObj == m_pThiefPartyLabel)
				continue;

			textObj->UpdateText(std::to_wstring(m_pPlayer->level[0]), L"LV. ");
			textObj->UpdateText(std::to_wstring(m_pPlayer->level[1]), L"LV. ");
			textObj->UpdateText(std::to_wstring(m_pPlayer->level[2]), L"LV. ");
		}
	}

	for (int i = 0; i < SKILL_COUNT; ++i)
	{
		float maxCD = m_fSkillMaxCooldown[i];
		float remCD = m_fSkillCooldown[i];

		bool onCooldown = (remCD > 0.0f);

		// 텍스트: 남은 초(소수점 1자리)
		if (m_pCooldownTexts[i])
		{
			m_pCooldownTexts[i]->SetVisible(onCooldown);
			if (onCooldown)
			{
				std::wstring cdStr = std::to_wstring((int)std::ceilf(remCD));
				m_pCooldownTexts[i]->UpdateText(cdStr, L"");
			}
		}
	}

	UpdateDebugOverlay();
}

void CScene::UpdateDebugOverlay()
{
	for (int i = 0; i < DEBUG_TEXT_LINES; ++i)
		if (m_pDebugTexts[i]) m_pDebugTexts[i]->SetVisible(m_bDebugMode);

	if (!m_bDebugMode) return;

	int nFireball = m_pFireballSystem ? (int)m_pFireballSystem->GetActiveParticles().size() : 0;
	int nHitSpark = m_pHitSparkSystem ? m_pHitSparkSystem->GetActiveCount() : 0;
	int nGreenSpirit = m_pGreenSpiritSystem ? m_pGreenSpiritSystem->GetActiveCount() : 0;
	int nDeathBurst = m_pDeathBurstSystem ? m_pDeathBurstSystem->GetActiveCount() : 0;
	int nTotalParticles = nFireball + nHitSpark + nGreenSpirit + nDeathBurst;

	int nConnected = 0;
	for (auto* p : m_vPlayers)
		if (p && p->isConnedted) ++nConnected;

	std::wstring bossStatus = m_pBoss ? (m_pBoss->IsDead() ? L"Dead" : L"Alive") : L"None";

	if (m_pDebugTexts[0]) m_pDebugTexts[0]->UpdateText(std::to_wstring(m_nCurrentFps), L"FPS: ");
	if (m_pDebugTexts[1]) m_pDebugTexts[1]->UpdateText(std::to_wstring(nTotalParticles), L"Particles: ");
	if (m_pDebugTexts[2]) m_pDebugTexts[2]->UpdateText(std::to_wstring((int)m_Monsters.size()) + L"  Boss: " + bossStatus, L"Monsters: ");
	if (m_pDebugTexts[3]) m_pDebugTexts[3]->UpdateText(std::to_wstring(nConnected) + L"/" + std::to_wstring(m_nOtherPlayers), L"Players: ");
	if (m_pDebugTexts[4]) m_pDebugTexts[4]->UpdateText(g_bAnimationBlendEnabled ? L"ON" : L"OFF (CapsLock)", L"Anim Blend: ");
}

void CScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CreateCbvSrvDescriptorHeaps(pd3dDevice, 100, 1000);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials(true);

	Device = pd3dDevice;
	Commandlist = pd3dCommandList;

	m_bEnableShadow = true;

	m_pSkyBox = new CSkyBox(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_pMap = new Map(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	// 터레인 보정 (1/4) Mesh Resolution
	float fScaleX = 533.2781f / 4096.0f;
	float fScaleZ = 534.9254f / 4096.0f;
	float fScaleY = 29.68098f;
	m_pTerrain = new CHeightMapTerrain(
		pd3dDevice,
		pd3dCommandList,
		m_pd3dGraphicsRootSignature,
		L"Terrain/HeightMap.raw",
		4097,
		4097,
		XMFLOAT3(fScaleX, fScaleY, fScaleZ),
		XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)
	);

	m_CollisionManager.InitializeDebugObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_CollisionManager.InitializeDamageNumberSystem(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_pFireballSystem = new CFireballSystem(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pGreenSpiritSystem = new CGreenSpiritSystem(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pHitSparkSystem = new CHitSparkSystem(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pDeathBurstSystem = new CDeathBurstSystem(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	CMonster::SetDeathBurstSystem(m_pDeathBurstSystem);
	m_pWeaponThrowSystem = new CWeaponThrowSystem();
	m_pWeaponThrowSystem->Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pBeamSystem = new CBeamSystem();
	m_pBeamSystem->Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pGroundCrackEffect = new CGroundCrackEffect();
	m_pGroundCrackEffect->Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_pSwordTrailEffect = new CSwordTrailEffect();
	m_pSwordTrailEffect->Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_pKnightModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Knight.bin", NULL);
	m_pWizardModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Wizard.bin", NULL);
	m_pThiefModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Thief.bin", NULL);

	m_Monsters.clear();
	for (const auto& desc : MONSTER_DESCS)
	{
		for (int i = 0; i < 3; ++i)
		{
			CMonster* monster = new CMonster(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, desc.modelPath, 5, nullptr, desc.hp, desc.startID + i);
			std::string path = desc.modelPath;
			size_t slash = path.rfind('/');
			size_t dot = path.rfind('.');
			std::string monsterName = path.substr(slash + 1, dot - slash - 1);
			monster->SetFrameName(monsterName.c_str());
			monster->SetPosition(XMFLOAT3(-99, -99, -99));
			m_Monsters.push_back(monster);
		}
	}

	m_pBoss = new CBossMonster(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature,
		"Model/Monster/DemonKingPA.bin", nullptr, 50000.f, 99999);
	//m_pBoss->SetFrameName("Boss");
	m_pBoss->SetTerrain(m_pTerrain);
	m_pBoss->SetPosition(XMFLOAT3(-9999.0f, -9999.0f, -9999.0f));
	// 이동 패킷의 LookAt 이후에도 유지되는 보스 전용 시각 크기.
	m_pBoss->SetVisualScale(3.0f);

	m_pGroundAttackRangeEffect = new CGroundAttackRangeEffect();
	m_pGroundAttackRangeEffect->Create(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, 4);
	// 이펙트의 소유권(생성/Animate/Render/Release)은 Scene이 그대로 가지고,
	// 보스는 포인터만 받아서 자신의 애니메이션 트랙(공격 패턴)에 따라 Spawn()만 호출한다.
	m_pBoss->SetGroundAttackRangeEffect(m_pGroundAttackRangeEffect);
	m_pBoss->SetDeathBurstSystem(m_pDeathBurstSystem);

	// otherplayer 설정
	m_nOtherPlayers = 6;
	m_ppOtherPlayers = new OtherPlayer * [m_nOtherPlayers];
	for (int i = 0; i < m_nOtherPlayers; ++i) m_ppOtherPlayers[i] = nullptr;
	m_vPlayers.clear();
	m_vPlayers.reserve(m_nOtherPlayers);

	g_knightIndex = 0;
	g_wizardIndex = 2;
	g_thiefIndex = 4;
	g_other_players.clear();

	for (int i = 0; i < 2; ++i) {
		m_ppOtherPlayers[i] = new OtherPlayer(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_pKnightModel);
		m_vPlayers.push_back(m_ppOtherPlayers[i]);
	}
	for (int i = 2; i < 4; ++i) {
		m_ppOtherPlayers[i] = new OtherPlayer(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_pWizardModel);
		m_vPlayers.push_back(m_ppOtherPlayers[i]);
	}
	for (int i = 4; i < 6; ++i) {
		m_ppOtherPlayers[i] = new OtherPlayer(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_pThiefModel);
		m_vPlayers.push_back(m_ppOtherPlayers[i]);
	}

	for (int i = 0; i < m_nOtherPlayers; ++i) {
		for (int j = 0; j < 7; ++j) {
			m_ppOtherPlayers[i]->m_pSkinnedAnimationController->SetTrackEnable(j, false);
			m_ppOtherPlayers[i]->m_pSkinnedAnimationController->SetTrackPosition(j, 0.0f);
		}
		m_ppOtherPlayers[i]->m_pSkinnedAnimationController->SetTrackEnable(0, true);
	}

	CInteractPrompt* pPressF = new CInteractPrompt(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature,
		L"Image/PressF.dds", XMFLOAT3(-9.0f, 1.0f, 35.0f)); // 위치, 기본 2.5 범위
	m_GameObjects.push_back(pPressF);

	// npc 생성
	CNPC* pNPC = new CNPC(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_GameObjects.push_back(pNPC);

	//
	//	m_GameObjects.clear();
	//	m_GameObjects.resize(8);

	//
#pragma region Items
//	long long itemIDs[8] = { 20000, 20001, 20002,
//							 30000, 30001, 30002, 30003, 30004};
//
//	float itemPrices[8] = { 80, 150, 80, 10, 20, 30, 40, 50 };
//
//	XMFLOAT3 positions[8] = {
//		{-2, 0, 19},
//		{-2, 0, 22},
//		{-2, 0, 25},
//		{-2, 0, 28},
//		{-2, 0, 31},
//		{-2, 0, 34},
//		{-2, 0, 37},
//		{-2, 0, 13}
//	};
//
//	CLoadedModelInfo* pFlashlightModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item/Flashlightgold.bin", NULL);
//	m_GameObjects[0] = new FlashLight(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pFlashlightModel);
//	m_GameObjects[0]->SetScale(3, 3, 3);
//	m_GameObjects[0]->Rotate(-90, 180, 0);
//	m_GameObjects[0]->SetFrameName("FlashLight");
//	m_GameObjects[0]->SetPosition(positions[0]);
//
//	static_cast<Item*>(m_GameObjects[0])->SetUniqueID(itemIDs[0]);
//	static_cast<Item*>(m_GameObjects[0])->SetPrice(itemPrices[0]);
//	g_items[itemIDs[0]] = static_cast<Item*>(m_GameObjects[0]);
//
//	if (pFlashlightModel) delete pFlashlightModel;
//
//	// ============================================================================================================
//
//	CLoadedModelInfo* pShovelModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item/Shovel.bin", NULL);
//	m_GameObjects[1] = new Shovel(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pShovelModel);
//	m_GameObjects[1]->SetScale(1, 1, 1);
//	m_GameObjects[1]->Rotate(0, -90, 160);
//	m_GameObjects[1]->SetFrameName("Shovel");
//	m_GameObjects[1]->SetPosition(positions[1]);
//
//	static_cast<Item*>(m_GameObjects[1])->SetUniqueID(itemIDs[1]);
//	static_cast<Item*>(m_GameObjects[1)->SetPrice(itemPrices[1]);
//	g_items[itemIDs[1]] = static_cast<Item*>(m_GameObjects[1]);
//
//	if (pShovelModel) delete pShovelModel;
//
//	// ============================================================================================================
//
//
//	CLoadedModelInfo* pWhistleModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item/Whistle.bin", NULL);
//	m_GameObjects[2] = new Whistle(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pWhistleModel);
//	m_GameObjects[2]->SetScale(1, 1, 1);
//	m_GameObjects[2]->SetFrameName("Whistle");
//	m_GameObjects[2]->SetPosition(positions[2]);
//
//	static_cast<Item*>(m_GameObjects[2])->SetUniqueID(itemIDs[2]);
//	static_cast<Item*>(m_GameObjects[2])->SetPrice(itemPrices[2]);
//	g_items[itemIDs[2]] = static_cast<Item*>(m_GameObjects[2]);
//
//	if (pWhistleModel) delete pWhistleModel;
//
//	// ============================================================================================================
//	// ============================================================================================================
//
//
//	CLoadedModelInfo* pGoldbarModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item/Goldbar.bin", NULL);
//	m_GameObjects[3] = new Whistle(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pGoldbarModel);
//	m_GameObjects[3]->SetScale(1, 1, 1);
//	m_GameObjects[3]->SetFrameName("Goldbar");
//	m_GameObjects[3]->SetPosition(positions[3]);
//
//	static_cast<Item*>(m_GameObjects[3])->SetUniqueID(itemIDs[3]);
//	static_cast<Item*>(m_GameObjects[3])->SetPrice(itemPrices[3]);
//	g_items[itemIDs[3]] = static_cast<Item*>(m_GameObjects[3]);
//
//	if (pGoldbarModel) delete pGoldbarModel;
//
//	// ============================================================================================================
//
//
//	CLoadedModelInfo* pCoinModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item/Coin.bin", NULL);
//	m_GameObjects[4] = new Whistle(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pCoinModel);
//	m_GameObjects[4]->SetScale(2, 2, 2);
//	m_GameObjects[4]->SetFrameName("Coin");
//	m_GameObjects[4]->SetPosition(positions[4]);
//
//	static_cast<Item*>(m_GameObjects[4])->SetUniqueID(itemIDs[4]);
//	static_cast<Item*>(m_GameObjects[4])->SetPrice(itemPrices[4]);
//	g_items[itemIDs[4]] = static_cast<Item*>(m_GameObjects[4]);
//
//	if (pCoinModel) delete pCoinModel;
//
//	// ============================================================================================================
//
//
//	CLoadedModelInfo* pCanister1Model = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item/Canisters_01.bin", NULL);
//	m_GameObjects[5] = new Whistle(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pCanister1Model);
//	m_GameObjects[5]->SetScale(1, 1, 1);
//	m_GameObjects[5]->SetFrameName("Canisters_01");
//	m_GameObjects[5]->SetPosition(positions[5]);
//
//	static_cast<Item*>(m_GameObjects[5])->SetUniqueID(itemIDs[5]);
//	static_cast<Item*>(m_GameObjects[5])->SetPrice(itemPrices[5]);
//	g_items[itemIDs[5]] = static_cast<Item*>(m_GameObjects[5]);
//
//	if (pCanister1Model) delete pCanister1Model;
//
//	// ============================================================================================================
//
//
//	CLoadedModelInfo* pCanister2Model = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item/Canisters_02.bin", NULL);
//	m_GameObjects[6] = new Whistle(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pCanister2Model);
//	m_GameObjects[6]->SetScale(1, 1, 1);
//	m_GameObjects[6]->SetFrameName("Canisters_02");
//	m_GameObjects[6]->SetPosition(positions[6]);
//
//	static_cast<Item*>(m_GameObjects[6])->SetUniqueID(itemIDs[6]);
//	static_cast<Item*>(m_GameObjects[6])->SetPrice(itemPrices[6]);
//	g_items[itemIDs[6]] = static_cast<Item*>(m_GameObjects[6]);
//
//	if (pCanister2Model) delete pCanister2Model;
//
//	// ============================================================================================================
//
//
//	CLoadedModelInfo* pCanister3Model = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item/Canisters_03.bin", NULL);
//	m_GameObjects[7] = new Whistle(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pCanister3Model);
//	m_GameObjects[7]->SetScale(1, 1, 1);
//	m_GameObjects[7]->SetFrameName("Canisters_03");
//	m_GameObjects[7]->SetPosition(positions[7]);
//
//	static_cast<Item*>(m_GameObjects[7])->SetUniqueID(itemIDs[7]);
//	static_cast<Item*>(m_GameObjects[7])->SetPrice(itemPrices[7]);
//	g_items[itemIDs[7]] = static_cast<Item*>(m_GameObjects[7]);
//
//	if (pCanister3Model) delete pCanister3Model;
//
//	m_nOtherPlayers = 1;
//	m_ppOtherPlayers = new OtherPlayer * [m_nOtherPlayers];
//	CLoadedModelInfo* pOtherPlayerModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Player.bin", NULL);
//	m_ppOtherPlayers[0] = new OtherPlayer(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pOtherPlayerModel);
//	m_ppOtherPlayers[0]->SetPosition(-1000, -1000, -1000);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackEnable(0, true);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackEnable(1, false);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackEnable(2, false);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackEnable(3, false);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackEnable(4, false);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackEnable(5, false);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackEnable(6, false);
//
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackPosition(1, 0.0f);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackPosition(2, 0.0f);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackPosition(3, 0.0f);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackPosition(4, 0.0f);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackPosition(5, 0.0f);
//	m_ppOtherPlayers[0]->m_pSkinnedAnimationController->SetTrackPosition(6, 0.0f);
//
//	if (pOtherPlayerModel) delete pOtherPlayerModel;
#pragma endregion
//
#pragma region InventoryUIandShop

//	// 인벤토리 UI 및 상점
//	m_Shaders.clear();
//	m_Shaders.resize(10);
//
//	CTexture* pTextureinven = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
//	pTextureinven->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/inven.dds", RESOURCE_TEXTURE2D, 0);
//	CreateShaderResourceViews(pd3dDevice, pTextureinven, 0, 15);
//	m_textureMap["inven"] = pTextureinven;
//
//	CTexture* pTextureItem1 = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
//	pTextureItem1->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Shovel.dds", RESOURCE_TEXTURE2D, 0);
//	CreateShaderResourceViews(pd3dDevice, pTextureItem1, 0, 15);
//	m_textureMap["Shovel"] = pTextureItem1;
//
//	CTexture* pTextureItem2 = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
//	pTextureItem2->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/FlashLight.dds", RESOURCE_TEXTURE2D, 0);
//	CreateShaderResourceViews(pd3dDevice, pTextureItem2, 0, 15);
//	m_textureMap["FlashLight"] = pTextureItem2;
//
//	CTexture* pTextureItem3 = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
//	pTextureItem3->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Whistle.dds", RESOURCE_TEXTURE2D, 0);
//	CreateShaderResourceViews(pd3dDevice, pTextureItem3, 0, 15);
//	m_textureMap["Whistle"] = pTextureItem3;
//
//	CTexture* pTextureItem4 = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
//	pTextureItem4->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Goldbar.dds", RESOURCE_TEXTURE2D, 0);
//	CreateShaderResourceViews(pd3dDevice, pTextureItem4, 0, 15);
//	m_textureMap["Goldbar"] = pTextureItem4;
//
//	CTexture* pTextureItem5 = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
//	pTextureItem5->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Coin.dds", RESOURCE_TEXTURE2D, 0);
//	CreateShaderResourceViews(pd3dDevice, pTextureItem5, 0, 15);
//	m_textureMap["Coin"] = pTextureItem5;
//
//	CTexture* pTextureItem6 = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
//	pTextureItem6->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Canisters_01.dds", RESOURCE_TEXTURE2D, 0);
//	CreateShaderResourceViews(pd3dDevice, pTextureItem6, 0, 15);
//	m_textureMap["Canisters_01"] = pTextureItem6;
//
//	CTexture* pTextureItem7 = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
//	pTextureItem7->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Canisters_02.dds", RESOURCE_TEXTURE2D, 0);
//	CreateShaderResourceViews(pd3dDevice, pTextureItem7, 0, 15);
//	m_textureMap["Canisters_02"] = pTextureItem7;
//
//	CTexture* pTextureItem8 = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
//	pTextureItem8->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Canisters_03.dds", RESOURCE_TEXTURE2D, 0);
//	CreateShaderResourceViews(pd3dDevice, pTextureItem8, 0, 15);
//	m_textureMap["Canisters_03"] = pTextureItem8;
//
//	CTextureToScreenShader* pTextureItem1Shader = new CTextureToScreenShader(1);
//	pTextureItem1Shader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
//	CScreenRectMeshTextured* pMesh = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, 0.02f, 0.225f * 0.5f, -0.65f, 0.4f * 0.5f);
//	pTextureItem1Shader->SetMesh(0, pMesh);
//	pTextureItem1Shader->SetTexture(pTextureinven);
//	m_Shaders[0] = pTextureItem1Shader;
//
//	CTextureToScreenShader* pTextureItem2Shader = new CTextureToScreenShader(1);
//	pTextureItem2Shader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
//	CScreenRectMeshTextured* pMesh1 = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, 0.02f + 0.125f, 0.225f * 0.5f, -0.65f, 0.4f * 0.5f);
//	pTextureItem2Shader->SetMesh(0, pMesh1);
//	pTextureItem2Shader->SetTexture(pTextureinven);
//	m_Shaders[1] = pTextureItem2Shader;
//
//	CTextureToScreenShader* pTextureItem3Shader = new CTextureToScreenShader(1);
//	pTextureItem3Shader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
//	CScreenRectMeshTextured* pMesh2 = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, 0.02f + 0.25f, 0.225f * 0.5f, -0.65f, 0.4f * 0.5f);
//	pTextureItem3Shader->SetMesh(0, pMesh2);
//	pTextureItem3Shader->SetTexture(pTextureinven);
//	m_Shaders[2] = pTextureItem3Shader;
//
//	CTextureToScreenShader* pTextureItem4Shader = new CTextureToScreenShader(1);
//	pTextureItem4Shader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
//	CScreenRectMeshTextured* pMesh3 = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, 0.02f + 0.375f, 0.225f * 0.5f, -0.65f, 0.4f * 0.5f);
//	pTextureItem4Shader->SetMesh(0, pMesh3);
//	pTextureItem4Shader->SetTexture(pTextureinven);
//	m_Shaders[3] = pTextureItem4Shader;
//
//	CTextureToScreenShader* pInventoryShader = new CTextureToScreenShader(1);
//	pInventoryShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
//	CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
//	pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Inventory.dds", RESOURCE_TEXTURE2D, 0);
//	CreateShaderResourceViews(pd3dDevice, pTexture, 0, 15);
//	CScreenRectMeshTextured* pInventoryMesh = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, -0.5f + 0.5f, 1.0f * 0.5f, -0.6f, 0.5f * 0.5f);
//	pInventoryShader->SetMesh(0, pInventoryMesh);
//	pInventoryShader->SetTexture(pTexture);
//	m_Shaders[4] = pInventoryShader;
//
//	//상점
//	CShopShader* pShopShader = new CShopShader(1);
//	pShopShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
//	pShopShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, NULL, NULL);
//
//	CTexture* pShopTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
//	pShopTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/shop.dds", RESOURCE_TEXTURE2D, 0);
//	CreateShaderResourceViews(pd3dDevice, pShopTexture, 0, 15);
//	CScreenRectMeshTextured* pShopMesh = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, -0.9f, 0.5f, 0.8f, 0.7f);
//	pShopShader->SetMesh(0, pShopMesh);
//	pShopShader->SetTexture(pShopTexture);
//	pShopShader->SetVisible(false);
//
//	m_Shaders[5] = pShopShader;
//
//	//상점 4칸
//	CTextureToScreenShader* pShopSpace1Shader = new CTextureToScreenShader(1);
//	pShopSpace1Shader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
//	CScreenRectMeshTextured* pShopMesh1 = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, -0.825f, 0.08f, 0.625f, 0.09f);
//	pShopSpace1Shader->SetMesh(0, pShopMesh1);
//	pShopSpace1Shader->SetTexture(pTextureinven);
//	pShopSpace1Shader->SetVisible(false);
//	m_Shaders[6] = pShopSpace1Shader;
//
//	CTextureToScreenShader* pShopSpace2Shader = new CTextureToScreenShader(1);
//	pShopSpace2Shader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
//	CScreenRectMeshTextured* pShopMesh2 = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, -0.825f, 0.08f, 0.505f, 0.09f);
//	pShopSpace2Shader->SetMesh(0, pShopMesh2);
//	pShopSpace2Shader->SetTexture(pTextureinven);
//	pShopSpace2Shader->SetVisible(false);
//	m_Shaders[7] = pShopSpace2Shader;
//
//	CTextureToScreenShader* pShopSpace3Shader = new CTextureToScreenShader(1);
//	pShopSpace3Shader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
//	CScreenRectMeshTextured* pShopMesh3 = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, -0.825f, 0.08f, 0.385f, 0.09f);
//	pShopSpace3Shader->SetMesh(0, pShopMesh3);
//	pShopSpace3Shader->SetTexture(pTextureinven);
//	pShopSpace3Shader->SetVisible(false);
//	m_Shaders[8] = pShopSpace3Shader;
//
//	CTextureToScreenShader* pShopSpace4Shader = new CTextureToScreenShader(1);
//	pShopSpace4Shader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
//	CScreenRectMeshTextured* pShopMesh4 = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, -0.825f, 0.08f, 0.265f, 0.09f);
//	pShopSpace4Shader->SetMesh(0, pShopMesh4);
//	pShopSpace4Shader->SetTexture(pTextureinven);
//	pShopSpace4Shader->SetVisible(false);
//	m_Shaders[9] = pShopSpace4Shader;
#pragma endregion

	CSoundManager::GetInstance()->PlayBGM("bgm_village");

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	CreateShadowResources(pd3dDevice, pd3dCommandList);
}

void CScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();
	if (m_pd3dCbvSrvDescriptorHeap) m_pd3dCbvSrvDescriptorHeap->Release();

	for (auto* obj : m_GameObjects)
	{
		if (obj) obj->Release();
	}
	m_GameObjects.clear();

	for (auto* shader : m_Shaders)
	{
		if (!shader) continue;
		shader->ReleaseShaderVariables();
		shader->ReleaseObjects();
		shader->Release();
	}
	m_Shaders.clear();

	if (m_pTerrain) delete m_pTerrain;
	if (m_pSkyBox) delete m_pSkyBox;
	if (m_pFireballSystem) { delete m_pFireballSystem; m_pFireballSystem = nullptr; }
	if (m_pGreenSpiritSystem) { delete m_pGreenSpiritSystem; m_pGreenSpiritSystem = nullptr; }
	if (m_pHitSparkSystem) { delete m_pHitSparkSystem; m_pHitSparkSystem = nullptr; }
	if (m_pDeathBurstSystem) { delete m_pDeathBurstSystem; m_pDeathBurstSystem = nullptr; }
	if (m_pWeaponThrowSystem) { delete m_pWeaponThrowSystem; m_pWeaponThrowSystem = nullptr; }
	if (m_pBeamSystem) { delete m_pBeamSystem; m_pBeamSystem = nullptr; }

	for (auto* monster : m_Monsters)
	{
		if (monster) monster->Release();
	}
	m_Monsters.clear();

	if (m_pBoss) { m_pBoss->Release(); m_pBoss = nullptr; }
	if (m_pGroundAttackRangeEffect) { delete m_pGroundAttackRangeEffect; m_pGroundAttackRangeEffect = nullptr; }

	for (auto* player : m_vPlayers)
	{
		if (player) player->Release();
	}
	m_vPlayers.clear();

	// m_UITextures에 담긴 텍스처들은 BuildSimpleUI()에서 pShader->SetTexture(pTexture)로
	// 이미 각 셰이더가 참조카운트(AddRef/Release)로 소유하고 있다.
	// 바로 위 m_Shaders 루프에서 shader->Release() -> ~CTextureToScreenShader()가
	// m_pTexture->Release()를 호출하면서 참조카운트가 0이 되어 이미 delete된 상태이므로,
	// 여기서 또 delete하면 이중 해제(double free)로 힙이 손상된다. 포인터 정리만 한다.
	m_UITextures.clear();

	ReleaseShaderVariables();

	if (m_pLights) delete[] m_pLights;
}

ID3D12RootSignature* CScene::CreateGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
	ID3D12RootSignature* pd3dGraphicsRootSignature = NULL;

	D3D12_DESCRIPTOR_RANGE pd3dDescriptorRanges[13];

	pd3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[0].NumDescriptors = 1;
	pd3dDescriptorRanges[0].BaseShaderRegister = 6; //t6: gtxtAlbedoTexture
	pd3dDescriptorRanges[0].RegisterSpace = 0;
	pd3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[1].NumDescriptors = 1;
	pd3dDescriptorRanges[1].BaseShaderRegister = 7; //t7: gtxtSpecularTexture
	pd3dDescriptorRanges[1].RegisterSpace = 0;
	pd3dDescriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[2].NumDescriptors = 1;
	pd3dDescriptorRanges[2].BaseShaderRegister = 8; //t8: gtxtNormalTexture
	pd3dDescriptorRanges[2].RegisterSpace = 0;
	pd3dDescriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[3].NumDescriptors = 1;
	pd3dDescriptorRanges[3].BaseShaderRegister = 9; //t9: gtxtMetallicTexture
	pd3dDescriptorRanges[3].RegisterSpace = 0;
	pd3dDescriptorRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[4].NumDescriptors = 1;
	pd3dDescriptorRanges[4].BaseShaderRegister = 10; //t10: gtxtEmissionTexture
	pd3dDescriptorRanges[4].RegisterSpace = 0;
	pd3dDescriptorRanges[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[5].NumDescriptors = 1;
	pd3dDescriptorRanges[5].BaseShaderRegister = 11; //t11: gtxtEmissionTexture
	pd3dDescriptorRanges[5].RegisterSpace = 0;
	pd3dDescriptorRanges[5].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[6].NumDescriptors = 1;
	pd3dDescriptorRanges[6].BaseShaderRegister = 12; //t12: gtxtEmissionTexture
	pd3dDescriptorRanges[6].RegisterSpace = 0;
	pd3dDescriptorRanges[6].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[7].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[7].NumDescriptors = 1;
	pd3dDescriptorRanges[7].BaseShaderRegister = 13; //t13: gtxtSkyBoxTexture
	pd3dDescriptorRanges[7].RegisterSpace = 0;
	pd3dDescriptorRanges[7].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[8].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[8].NumDescriptors = 1;
	pd3dDescriptorRanges[8].BaseShaderRegister = 1; //t1: gtxtTerrainBaseTexture
	pd3dDescriptorRanges[8].RegisterSpace = 0;
	pd3dDescriptorRanges[8].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[9].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[9].NumDescriptors = 1;
	pd3dDescriptorRanges[9].BaseShaderRegister = 2; //t2: gtxtTerrainDetailTexture
	pd3dDescriptorRanges[9].RegisterSpace = 0;
	pd3dDescriptorRanges[9].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[10].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[10].NumDescriptors = 1;
	pd3dDescriptorRanges[10].BaseShaderRegister = 0; //t0: gtxTexture
	pd3dDescriptorRanges[10].RegisterSpace = 0;
	pd3dDescriptorRanges[10].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[11].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[11].NumDescriptors = 1;
	pd3dDescriptorRanges[11].BaseShaderRegister = 3; //t3: gFontTexture
	pd3dDescriptorRanges[11].RegisterSpace = 0;
	pd3dDescriptorRanges[11].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[12].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[12].NumDescriptors = 1;
	pd3dDescriptorRanges[12].BaseShaderRegister = 5; // t5
	pd3dDescriptorRanges[12].RegisterSpace = 0;
	pd3dDescriptorRanges[12].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER pd3dRootParameters[20];

	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[0].Descriptor.ShaderRegister = 1; //Camera
	pd3dRootParameters[0].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[1].Constants.Num32BitValues = 33;
	pd3dRootParameters[1].Constants.ShaderRegister = 2; //GameObject
	pd3dRootParameters[1].Constants.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[2].Descriptor.ShaderRegister = 4; //Lights
	pd3dRootParameters[2].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[3].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[0]);
	pd3dRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[4].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[1]);
	pd3dRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[5].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[2]);
	pd3dRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[6].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[6].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[3]);
	pd3dRootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[7].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[7].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[4]);
	pd3dRootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[8].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[8].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[5]);
	pd3dRootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[9].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[9].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[6]);
	pd3dRootParameters[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[10].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[10].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[7]);
	pd3dRootParameters[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[11].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[11].Descriptor.ShaderRegister = 7; //Skinned Bone Offsets
	pd3dRootParameters[11].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[11].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	pd3dRootParameters[12].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[12].Descriptor.ShaderRegister = 8; //Skinned Bone Transforms
	pd3dRootParameters[12].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[12].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	pd3dRootParameters[13].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[13].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[13].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[8]);
	pd3dRootParameters[13].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[14].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[14].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[14].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[9]);
	pd3dRootParameters[14].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[15].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[15].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[15].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[10]);
	pd3dRootParameters[15].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[16].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[16].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[16].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[11]);
	pd3dRootParameters[16].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[17].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[17].DescriptorTable.NumDescriptorRanges = 1; //t5
	pd3dRootParameters[17].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[12]);
	pd3dRootParameters[17].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[18].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[18].Descriptor.ShaderRegister = 5; // b5
	pd3dRootParameters[18].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[18].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[19].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	pd3dRootParameters[19].Descriptor.ShaderRegister = 4;
	pd3dRootParameters[19].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[19].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC pd3dSamplerDescs[4];

	pd3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].MipLODBias = 0;
	pd3dSamplerDescs[0].MaxAnisotropy = 1;
	pd3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[0].MinLOD = 0;
	pd3dSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[0].ShaderRegister = 0;
	pd3dSamplerDescs[0].RegisterSpace = 0;
	pd3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].MipLODBias = 0;
	pd3dSamplerDescs[1].MaxAnisotropy = 1;
	pd3dSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[1].MinLOD = 0;
	pd3dSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[1].ShaderRegister = 1;
	pd3dSamplerDescs[1].RegisterSpace = 0;
	pd3dSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[2].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[2].MipLODBias = 0;
	pd3dSamplerDescs[2].MaxAnisotropy = 1;
	pd3dSamplerDescs[2].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[2].MinLOD = 0;
	pd3dSamplerDescs[2].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[2].ShaderRegister = 2;
	pd3dSamplerDescs[2].RegisterSpace = 0;
	pd3dSamplerDescs[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[3].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	pd3dSamplerDescs[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[3].MipLODBias = 0;
	pd3dSamplerDescs[3].MaxAnisotropy = 1;
	pd3dSamplerDescs[3].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	pd3dSamplerDescs[3].MinLOD = 0;
	pd3dSamplerDescs[3].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[3].ShaderRegister = 3; // s3
	pd3dSamplerDescs[3].RegisterSpace = 0;
	pd3dSamplerDescs[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = _countof(pd3dSamplerDescs);
	d3dRootSignatureDesc.pStaticSamplers = pd3dSamplerDescs;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return(pd3dGraphicsRootSignature);
}

void CScene::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(LIGHTS) + 255) & ~255); //256의 배수
	m_pd3dcbLights = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbLights->Map(0, NULL, (void**)&m_pcbMappedLights);
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	::memcpy(m_pcbMappedLights->m_pLights, m_pLights, sizeof(LIGHT) * m_nLights);
	::memcpy(&m_pcbMappedLights->m_xmf4GlobalAmbient, &m_xmf4GlobalAmbient, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedLights->m_nLights, &m_nLights, sizeof(int));
}

void CScene::ReleaseShaderVariables()
{
	if (m_pd3dcbLights)
	{
		m_pd3dcbLights->Unmap(0, NULL);
		m_pd3dcbLights->Release();
	}
}

void CScene::ReleaseUploadBuffers()
{
	if (m_pSkyBox) m_pSkyBox->ReleaseUploadBuffers();
	if (m_pTerrain) m_pTerrain->ReleaseUploadBuffers();
	if (m_pMap) m_pMap->ReleaseUploadBuffers();
	if (m_pBoss) m_pBoss->ReleaseUploadBuffers();
	if (m_pGroundAttackRangeEffect) m_pGroundAttackRangeEffect->ReleaseUploadBuffers();
	for (auto* shader : m_Shaders) if (shader) shader->ReleaseUploadBuffers();
	for (auto* obj : m_GameObjects) if (obj) obj->ReleaseUploadBuffers();
	for (auto* monster : m_Monsters) if (monster) monster->ReleaseUploadBuffers();
	for (auto* otherplayer : m_vPlayers) if (otherplayer) otherplayer->ReleaseUploadBuffers();
}

void CScene::CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews)
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	d3dDescriptorHeapDesc.NumDescriptors = nConstantBufferViews + nShaderResourceViews; //CBVs + SRVs 
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dCbvSrvDescriptorHeap);

	m_d3dCbvCPUDescriptorNextHandle = m_d3dCbvCPUDescriptorStartHandle = m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_d3dCbvGPUDescriptorNextHandle = m_d3dCbvGPUDescriptorStartHandle = m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	m_d3dSrvCPUDescriptorNextHandle.ptr = m_d3dSrvCPUDescriptorStartHandle.ptr = m_d3dCbvCPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);
	m_d3dSrvGPUDescriptorNextHandle.ptr = m_d3dSrvGPUDescriptorStartHandle.ptr = m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);
}

D3D12_GPU_DESCRIPTOR_HANDLE CScene::CreateConstantBufferViews(ID3D12Device* pd3dDevice, int nConstantBufferViews, ID3D12Resource* pd3dConstantBuffers, UINT nStride)
{
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = m_d3dCbvGPUDescriptorNextHandle;
	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = pd3dConstantBuffers->GetGPUVirtualAddress();
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	for (int j = 0; j < nConstantBufferViews; j++)
	{
		d3dCBVDesc.BufferLocation = d3dGpuVirtualAddress + (nStride * j);
		m_d3dCbvCPUDescriptorNextHandle.ptr = m_d3dCbvCPUDescriptorNextHandle.ptr + ::gnCbvSrvDescriptorIncrementSize;
		pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_d3dCbvCPUDescriptorNextHandle);
		m_d3dCbvGPUDescriptorNextHandle.ptr = m_d3dCbvGPUDescriptorNextHandle.ptr + ::gnCbvSrvDescriptorIncrementSize;
	}
	return(d3dCbvGPUDescriptorHandle);
}

void CScene::CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex)
{
	m_d3dSrvCPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);
	m_d3dSrvGPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);

	if (pTexture)
	{
		int nTextures = pTexture->GetTextures();
		for (int i = 0; i < nTextures; i++)
		{
			ID3D12Resource* pShaderResource = pTexture->GetResource(i);
			D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(i);
			pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_d3dSrvCPUDescriptorNextHandle);
			m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
			pTexture->SetGpuDescriptorHandle(i, m_d3dSrvGPUDescriptorNextHandle);
			m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
		}
	}
	int nRootParameters = pTexture->GetRootParameters();
	for (int j = 0; j < nRootParameters; j++) pTexture->SetRootParameterIndex(j, nRootParameterStartIndex + j);
}

void CScene::CreateShadowResources(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// 1) ShadowMap (R32 typeless)
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = SHADOW_MAP_SIZE;
	desc.Height = SHADOW_MAP_SIZE;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	HRESULT hr = pd3dDevice->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 시작은 SRV 상태로 두고, 패스 시작 때 DEPTH_WRITE로 배리어
		&clearValue,
		__uuidof(ID3D12Resource), (void**)&m_pd3dShadowMap);

	// 2) DSV Heap (1개)
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	pd3dDevice->CreateDescriptorHeap(&dsvHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dShadowDsvHeap);

	m_d3dShadowDSV = m_pd3dShadowDsvHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	pd3dDevice->CreateDepthStencilView(m_pd3dShadowMap, &dsvDesc, m_d3dShadowDSV);

	// 3) SRV 생성 (CBV/SRV heap의 고정 index에 생성)
	D3D12_CPU_DESCRIPTOR_HANDLE srvCPU = m_d3dSrvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE srvGPU = m_d3dSrvGPUDescriptorStartHandle;

	srvCPU.ptr += ::gnCbvSrvDescriptorIncrementSize * SHADOW_SRV_INDEX;
	srvGPU.ptr += ::gnCbvSrvDescriptorIncrementSize * SHADOW_SRV_INDEX;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	pd3dDevice->CreateShaderResourceView(m_pd3dShadowMap, &srvDesc, srvCPU);
	m_d3dShadowSRV = srvGPU;

	// 4) Shadow CB (UPLOAD)
	UINT cbBytes = (sizeof(CB_SHADOW_INFO) + 255) & ~255;
	m_pd3dcbShadow = ::CreateBufferResource(pd3dDevice, pd3dCommandList, nullptr, cbBytes,
		D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr);
	m_pd3dcbShadow->Map(0, nullptr, (void**)&m_pcbMappedShadow);

	// 5) Shadow viewport/scissor
	m_ShadowViewport = { 0.0f, 0.0f, (float)SHADOW_MAP_SIZE, (float)SHADOW_MAP_SIZE, 0.0f, 1.0f };
	m_ShadowScissor = { 0, 0, (LONG)SHADOW_MAP_SIZE, (LONG)SHADOW_MAP_SIZE };

	// 6) Shadow PSO 생성
	m_pShadowShader = new CShadowShader();
	m_pShadowShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_pSkinnedShadowShader = new CSkinnedShadowShader();
	m_pSkinnedShadowShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
}

void CScene::ShowMissionText(const std::wstring& text)
{
	// CFontMesh는 x, y를 그대로 정점(NDC, clip-space) 좌표로 사용한다.
	// mission.dds 패널은 BuildSimpleUI에서 {left=0.35, top=0.5, width=0.25, height=0.5}
	// (0~1, 좌상단 기준) 으로 배치되는데, 이를 NDC로 환산하면
	//   x: left*2-1 ~ (left+width)*2-1  =>  -0.3 ~ 0.2
	//   y: 1-top*2  ~ 1-(top+height)*2  =>   0.0 ~ -1.0
	// 이 범위 안(패널 좌측 상단 부근)에 텍스트가 찍히도록 좌표를 잡는다.
	constexpr float MISSION_TEXT_X = -0.9f;
	constexpr float MISSION_TEXT_Y = -0.5f;

	if (!m_pMissionText)
	{
		m_pMissionText = new CText(Device, Commandlist, m_pd3dGraphicsRootSignature,
			text, MISSION_TEXT_X, MISSION_TEXT_Y);

		// 쿨타임 텍스트와 동일하게 m_GameObjects 렌더 루프를 타도록 등록한다.
		// (RenderImpl에서 별도로 수동 Render() 호출하던 방식은 제거)
		m_GameObjects.push_back(m_pMissionText);
	}
	else
	{
		// UpdateText(text, fixText) 형태라 fixText는 빈 문자열로
		m_pMissionText->UpdateText(text, L"");
	}

	m_pMissionText->SetVisible(true);

	if (m_pMissionBgShader)
		m_pMissionBgShader->SetVisible(true);

	m_bMissionUIVisible = true;
}

void CScene::HideMissionText()
{
	m_bMissionUIVisible = false;

	if (m_pMissionText)
		m_pMissionText->SetVisible(false);

	HideMissionProgress();

	if (m_pMissionBgShader)
		m_pMissionBgShader->SetVisible(false);
}

void CScene::ShowMissionProgress(int currentCount, int targetCount)
{
	constexpr float MISSION_PROGRESS_X = -0.9f;
	constexpr float MISSION_PROGRESS_Y = -0.6f;
	const std::wstring progress = L"Progress: " + std::to_wstring(currentCount) + L"/" + std::to_wstring(targetCount);

	if (!m_pMissionProgressText)
	{
		m_pMissionProgressText = new CText(Device, Commandlist, m_pd3dGraphicsRootSignature,
			progress, MISSION_PROGRESS_X, MISSION_PROGRESS_Y);
		m_GameObjects.push_back(m_pMissionProgressText);
	}
	else
	{
		m_pMissionProgressText->UpdateText(progress, L"");
	}

	m_pMissionProgressText->SetVisible(true);
}

void CScene::HideMissionProgress()
{
	if (m_pMissionProgressText)
		m_pMissionProgressText->SetVisible(false);
}

void CScene::TriggerSkillCooldown(int skillIndex)
{
	if (skillIndex < 0 || skillIndex >= SKILL_COUNT) return;
	m_fSkillMaxCooldown[skillIndex] = CalcMaxCooldown(skillIndex);
	m_fSkillCooldown[skillIndex] = m_fSkillMaxCooldown[skillIndex];
}

float CScene::CalcMaxCooldown(int skillIndex) const
{
	if (!m_pPlayer) return SKILL_BASE_CD[skillIndex];
	int lv = m_pPlayer->level[skillIndex]; // 0-based
	// 레벨당 5% 감소, 최대 70% 감소(lv 14)
	float factor = 1.0f - std::min(lv * 0.05f, 0.70f);
	return SKILL_BASE_CD[skillIndex] * factor;
}

void CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	RECT rt[3] =
	{
		{ 1200, 800, 1400, 1000 },
		{ 1450, 800, 1650, 1000 },
		{ 1700, 800, 1900, 1000 },
	};

	POINT pt;
	GetCursorPos(&pt);        // screen 좌표
	ScreenToClient(hWnd, &pt); // client 좌표로 변환

	auto* p = dynamic_cast<CTerrainPlayer*>(m_pPlayer);

	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	{
		if (!p) break;

		int idx = -1;
		for (int i = 0; i < 3; ++i)
			if (PtInRect(&rt[i], pt)) { idx = i; break; }

		if (idx == -1)
		{
			p->m_currentAnim = AnimationState::ATTACK;
			if (m_pModel == NULL)
			{
				break;
			}
			if (m_pModel == m_pKnightModel)
			{
				int randomIndex = (rand() % 2) + 1;
				std::string soundName = "knight_attack_" + std::to_string(randomIndex);
				CSoundManager::GetInstance()->PlaySFX(soundName);
			}
			else if (m_pModel == m_pWizardModel)
			{
				CSoundManager::GetInstance()->PlaySFX("wizard_attack");
			}
			else if (m_pModel == m_pThiefModel)
			{
				CSoundManager::GetInstance()->PlaySFX("rogue_attack");
			}
			break;
		}

		int lv = p->level[idx];
		int cost = 100 + lv * 50;
		int prob = 80 - lv * 10; if (prob < 10) prob = 10;

		if (m_pPlayer->Pgold >= cost) {
			m_pPlayer->Pgold -= cost;

			cs_packet_use_gold pkt{};
			pkt.size = sizeof(pkt);
			pkt.type = CS_P_USE_GOLD;
			pkt.amount = cost;
			send_packet(&pkt);

			if (Chance(prob)) {
				p->level[idx]++;

				SkillSlot slot = static_cast<SkillSlot>(idx);
				send_skill_upgrade(slot);
				cout << "[스킬강화 성공] slot=" << idx << " 새레벨=" << p->level[idx] << "\n";
			}
			else {
				cout << "[스킬강화 실패] slot=" << idx << "\n";
			}

		}
	}
	break;
	case WM_RBUTTONDOWN:
	{
		if (!IsSkillOnCooldown(0)) {
			p->m_currentAnim = AnimationState::SKILL1;
			if (m_pModel == m_pWizardModel)
			{
				CGameObject* pHand = p->FindFrame("RightHand");

				if (!pHand) break;

				// 위치/방향 변수로 먼저 받아두기
				XMFLOAT3 firePos = pHand->GetPosition();
				XMFLOAT3 fireLook = p->GetLook();

				// 로컬 이펙트 실행
				m_pFireballSystem->Emit(firePos, fireLook, 20.0f);

				std::cout << "[SKILL] 파이어볼 송신 | pos=("
					<< firePos.x << ", " << firePos.y << ", " << firePos.z
					<< ") look=(" << fireLook.x << ", " << fireLook.y << ", " << fireLook.z << ")\n";

				CSoundManager::GetInstance()->PlaySFX("wizard_rk");

				//m_pFireballSystem->Emit(pHand->GetPosition(), p->GetLook(), 20.0f); 

				// server!!
				send_skill_packet(firePos, fireLook);
			}
			else if (m_pModel == m_pThiefModel)
			{
				if (m_pWeaponThrowSystem->IsActive()) break;

				CGameObject* pWeapon = p->FindFrame("SM_Weapon_01");
				if (!pWeapon) break;

				XMFLOAT3 throwPos = pWeapon->GetPosition();
				XMFLOAT3 throwDir = p->GetLook();

				m_pWeaponThrowSystem->Emit(throwPos, throwDir, 30.0f, pWeapon);

				std::cout << "[SKILL] 도적 무기 던지기 | pos=("
					<< throwPos.x << ", " << throwPos.y << ", " << throwPos.z
					<< ") dir=(" << throwDir.x << ", " << throwDir.y << ", " << throwDir.z << ")\n";

				CSoundManager::GetInstance()->PlaySFX("rogue_rk");

				send_weapon_pos_packet(throwPos, throwDir);
			}
			else if (m_pModel == m_pKnightModel)
			{
				int randomIndex = (rand() % 2) + 1;
				std::string soundName = "knight_rk_" + std::to_string(randomIndex);
				CSoundManager::GetInstance()->PlaySFX(soundName);
				std::cout << "[SKILL] 기사 방패막기 시작\n";

				send_shield_block_packet(true);
			}
			else {
				// 기사 방패막기이므로 몬스터 공격 X 처리
			}
			TriggerSkillCooldown(0);
		}
	}
	break;
	case WM_RBUTTONUP:
	{
		if (m_pModel == m_pKnightModel)
		{
			std::cout << "[SKILL] 기사 방패막기 종료\n";
			send_shield_block_packet(false);
		}
	}
	break;
	}
}

void CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	auto* pPlayer = dynamic_cast<CTerrainPlayer*>(m_pPlayer);
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam) {
		case 'F':
		{
			bool bInteractedNPC = false;

			for (auto* obj : m_GameObjects)
			{
				// 보스맵 가기위한 작업
				if (auto* pPrompt = dynamic_cast<CInteractPrompt*>(obj))
					if (pPrompt->IsInRange() && pPlayer) {
						pPlayer->SetPosition(XMFLOAT3(219, 5, 18));
						m_pBoss->ToggleHpbarVisible();
					}

				// NPC위치와 플레이어 위치가 가까우면 미션 요청
				if (auto* pNPC = dynamic_cast<CNPC*>(obj))
				{
					if (!pPlayer) continue;

					XMFLOAT3 npcPos = pNPC->GetPosition();
					XMFLOAT3 playerPos = pPlayer->GetPosition();
					float distance = sqrt(pow(npcPos.x - playerPos.x, 2) + pow(npcPos.y - playerPos.y, 2) + pow(npcPos.z - playerPos.z, 2));
					if (distance < 5.0f) {
						// 서버로 상호작용 요청만 보냄. 실제 텍스트 표시는
						// Network.cpp의 SC_P_NPC_MISSION 수신 핸들러에서 ShowMissionText() 호출로 처리됨.
						send_npc_interact_packet(0);
						bInteractedNPC = true;
					}
				}
			}

			// NPC와 상호작용한 프레임에는 보스 체력바 토글(디버그용)을 건너뜀
			if (!bInteractedNPC && m_pBoss)
				m_pBoss->ToggleHpbarVisible();
			break;
		}

		case VK_TAB:
			m_bEnableShadow = !m_bEnableShadow;
			break;
		case 'Q':
			if (!IsSkillOnCooldown(1)) {
				pPlayer->m_currentAnim = AnimationState::SKILL2;
				// 법사 otherplayer 공격력 늘리기
				if (m_pModel == m_pWizardModel) {

					send_buff_atk_packet();

					for (auto& kv : g_other_player_slots)
					{
						long long player_id = kv.first;
						int slot = kv.second;
						OtherPlayer* otherPlayer = m_ppOtherPlayers[slot];
						if (!otherPlayer) continue;
						otherPlayer->damage += (pPlayer->level[2]); // 다른 플레이어 공격력 증가
						m_pBeamSystem->Emit(otherPlayer->GetPosition(), pPlayer->GetPosition());
					}

					CSoundManager::GetInstance()->PlaySFX("wizard_q");
				}
				// 이부분 기사 q 스킬
				else if (m_pModel == m_pKnightModel)
				{
					XMFLOAT3 pos = pPlayer->GetPosition();
					XMFLOAT3 look = pPlayer->GetLook();

					if (m_pGroundCrackEffect)
						m_pGroundCrackEffect->Trigger(pos, look);

					CSoundManager::GetInstance()->PlaySFX("knight_q");

					send_strike_packet(pos, look);
				}
				// Thief Q: spinning slash - close-range AoE hit reusing the same generic
				// strike protocol as the knight's ground crack, with its own visual/sound.
				else if (m_pModel == m_pThiefModel)
				{
					XMFLOAT3 pos = pPlayer->GetPosition();
					XMFLOAT3 look = pPlayer->GetLook();

					if (m_pSwordTrailEffect)
					{
						// Full-circle glowing motion trail (procedural ribbon mesh, not particles) for the spin slash.
						XMFLOAT3 sweepOrigin = pos;
						sweepOrigin.y += 1.0f;
						XMFLOAT3 right = pPlayer->GetRight();
						m_pSwordTrailEffect->Trigger(sweepOrigin, look, right, 2.2f, 360.0f, 0.5f);
					}

					CSoundManager::GetInstance()->PlaySFX("rogue_q");

					send_strike_packet(pos, look);
				}

				TriggerSkillCooldown(1);
			}
			break;

		case 'E':
			if (!IsSkillOnCooldown(2)) {
				pPlayer->m_currentAnim = AnimationState::SKILL3;
				if (m_pModel == m_pWizardModel) {
					if (m_pGreenSpiritSystem)
					{
						XMFLOAT3 footPos = pPlayer->GetPosition();
						footPos.y -= 0.5f;
						m_pGreenSpiritSystem->Emit(footPos);

						CSoundManager::GetInstance()->PlaySFX("wizard_e");

						//SERVER!!
						send_buff_hp_packet();
					}
				}
				else if (m_pModel == m_pKnightModel) {
					CSoundManager::GetInstance()->PlaySFX("knight_e");

					send_taunt_packet(pPlayer->level[2] * 5); //도발범위는 플레이어 레벨에 따라 증가
					// 기사 도발	
					// 몬스터들 공격 멈추고 lookat = 기사 위치로
					// m_pLevel[2]의 값에 따라 도발 지속시간 증가
				}
				else if (m_pModel == m_pThiefModel) {
					const float SEARCH_RANGE = pPlayer->level[2] * 5;  // 탐색 범위
					const float BEHIND_OFFSET = 2.0f;  // 몬스터 뒤 얼마나 멀리

					XMFLOAT3 playerPos = pPlayer->GetPosition();
					XMVECTOR vPlayer = XMLoadFloat3(&playerPos);

					CMonster* pNearest = nullptr;
					float fMinDist = SEARCH_RANGE;

					for (auto* monster : m_Monsters) {
						if (!monster || monster->IsDead()) continue;

						XMFLOAT3 monPos = monster->GetPosition();
						XMVECTOR vMon = XMLoadFloat3(&monPos);
						float dist = XMVectorGetX(XMVector3Length(XMVectorSubtract(vMon, vPlayer)));

						if (dist < fMinDist) {
							fMinDist = dist;
							pNearest = monster;
						}
					}

					if (pNearest) {
						XMFLOAT3 monPos = pNearest->GetPosition();
						XMFLOAT3 monLook = pNearest->GetLook();  // 몬스터가 바라보는 방향

						// 몬스터 뒤쪽 = 몬스터 위치 - (몬스터 look * offset)
						XMVECTOR vMonPos = XMLoadFloat3(&monPos);
						XMVECTOR vMonLook = XMLoadFloat3(&monLook);
						vMonLook = XMVector3Normalize(vMonLook);

						XMVECTOR vBehind = XMVectorSubtract(vMonPos, XMVectorScale(vMonLook, BEHIND_OFFSET));

						XMFLOAT3 teleportPos;
						XMStoreFloat3(&teleportPos, vBehind);
						teleportPos.y = playerPos.y;  // y축은 플레이어 그대로 유지

						// 2) 플레이어 위치 이동
						pPlayer->SetPosition(teleportPos);

						// 3) 플레이어가 몬스터를 바라보도록 회전
						//    방향: teleportPos -> monPos
						XMVECTOR vDir = XMVectorSubtract(XMLoadFloat3(&monPos), vBehind);
						vDir = XMVector3Normalize(vDir);
						XMFLOAT3 faceDir;
						XMStoreFloat3(&faceDir, vDir);

						// y축 회전각 계산 후 플레이어 방향 설정
						float yaw = atan2f(faceDir.x, faceDir.z);  // XZ 평면 각도
						pPlayer->Rotate(0.0f, XMConvertToDegrees(yaw), 0.0f);
					}
					CSoundManager::GetInstance()->PlaySFX("rogue_q");
				}
				TriggerSkillCooldown(2);
			}
			break;
		case 'P':
			m_bDebugMode = !m_bDebugMode;
			break;

		case VK_CAPITAL:
			g_bAnimationBlendEnabled = !g_bAnimationBlendEnabled;
			break;
		}
		break;
	}
}

bool CScene::ProcessInput(UCHAR* pKeysBuffer)
{
	return(false);
}

void CScene::AnimateObjects(float fTimeElapsed)
{
	{
		std::vector<PendingBossDeath> pendingDeaths;
		{
			std::lock_guard<std::mutex> lock(g_pendingBossMutex);
			if (!g_pendingBossDeaths.empty()) pendingDeaths.swap(g_pendingBossDeaths);
		}
		for (const auto& death : pendingDeaths)
		{
			if (!m_pBoss || death.bossID != m_pBoss->GetMonsterID()) continue;
			if (m_pBoss->GetState() == BossState::Death) continue;
			m_pBoss->SetHP(0.0f);
			m_pBoss->SetHpbarVisible(false);
			if (m_pDeathBurstSystem) m_pDeathBurstSystem->Emit(m_pBoss->GetPosition());
			m_pBoss->TransitionTo(BossState::Death);
			CSoundManager::GetInstance()->PlaySFX("boss_die_1");
			CSoundManager::GetInstance()->PlayBGM("bgm_winner");
			std::cout << "[BOSS] Death applied on main thread, killer=" << death.killerID << "\n";
		}
	}
	// 네트워크 스레드가 큐에 쌓아둔 미션 텍스트를 메인 스레드에서 안전하게 반영.
	// (ShowMissionText 내부에서 D3D12 리소스를 만들고 m_GameObjects를 건드리므로
	//  반드시 이 루프보다 먼저, 그리고 메인 스레드에서만 호출되어야 함)
	{
		std::vector<PendingMissionText> pending;
		{
			std::lock_guard<std::mutex> lock(g_pendingMissionMutex);
			if (!g_pendingMissionTexts.empty())
			{
				pending.swap(g_pendingMissionTexts);
			}
		}
		for (auto& p : pending)
		{
			switch (p.type)
			{
			case PendingMissionUiType::Info:
				ShowMissionText(p.text);
				break;
			case PendingMissionUiType::Progress:
				ShowMissionProgress(p.currentCount, p.targetCount);
				break;
			case PendingMissionUiType::Complete:
				ShowMissionText(p.text);
				HideMissionProgress();
				break;
			}
		}
	}

	m_CollisionManager.UpdateDamageNumbers(fTimeElapsed);

	for (auto* obj : m_GameObjects) {
		if (obj) obj->Animate(fTimeElapsed);

		if (auto* pPrompt = dynamic_cast<CInteractPrompt*>(obj)) {
			if (m_pPlayer) pPrompt->Update(m_pPlayer->GetPosition());
		}
	}

	for (int i = 0; i < SKILL_COUNT; ++i)
	{
		// 레벨이 바뀌면 최대 쿨타임 재계산
		float newMax = CalcMaxCooldown(i);
		if (std::fabsf(newMax - m_fSkillMaxCooldown[i]) > 0.01f)
			m_fSkillMaxCooldown[i] = newMax;

		// 남은 쿨타임 차감
		if (m_fSkillCooldown[i] > 0.0f)
		{
			m_fSkillCooldown[i] -= fTimeElapsed;
			if (m_fSkillCooldown[i] < 0.0f)
				m_fSkillCooldown[i] = 0.0f;
		}
	}
	m_fElapsedTime += fTimeElapsed;

	if (m_pBoss) { m_pBoss->Animate(fTimeElapsed); m_pBoss->Update(fTimeElapsed); }

	// 보스가 죽으면 3초간 대기했다가 엔딩 씬으로 전환한다.
	if (m_pBoss && m_pBoss->IsDead() && !m_bEndSceneRequested)
	{
		m_fBossDeathTimer += fTimeElapsed;
		if (m_fBossDeathTimer >= BOSS_DEATH_TO_END_DELAY)
		{
			m_bEndSceneRequested = true; // 중복 요청 방지
			gGameFramework.SetClearTime(m_fElapsedTime); // EndScene에 표시할 클리어 타임 저장
			gGameFramework.RequestMoveToScene(3); // CEndScene
		}
	}
	if (m_pGroundAttackRangeEffect) m_pGroundAttackRangeEffect->Animate(fTimeElapsed);
	if (m_pFireballSystem) m_pFireballSystem->Animate(fTimeElapsed);
	if (m_pGreenSpiritSystem) m_pGreenSpiritSystem->Animate(fTimeElapsed);
	if (m_pHitSparkSystem) m_pHitSparkSystem->Animate(fTimeElapsed);
	if (m_pDeathBurstSystem) m_pDeathBurstSystem->Animate(fTimeElapsed);
	if (m_pWeaponThrowSystem) m_pWeaponThrowSystem->Animate(fTimeElapsed);
	if (m_pBeamSystem) m_pBeamSystem->Animate(fTimeElapsed);
	if (m_pGroundCrackEffect) m_pGroundCrackEffect->Update(fTimeElapsed);
	if (m_pSwordTrailEffect) m_pSwordTrailEffect->Update(fTimeElapsed);

	for (auto* shader : m_Shaders) if (shader) shader->AnimateObjects(fTimeElapsed);

	UpdatePartyHPUI();

	++m_nFpsFrameCount;
	m_fFpsAccumTime += fTimeElapsed;
	if (m_fFpsAccumTime >= 1.0f)
	{
		m_nCurrentFps = m_nFpsFrameCount;
		m_nFpsFrameCount = 0;
		m_fFpsAccumTime -= 1.0f;
	}
}

void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	m_bHasCurrentRT = false;
	RenderImpl(pd3dCommandList, pCamera);
}

void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv)
{
	// 현재 프레임 RT 보관
	m_CurrentRTV = rtv;
	m_CurrentDSV = dsv;
	m_bHasCurrentRT = true;

	RenderShadowPass(pd3dCommandList);

	pd3dCommandList->OMSetRenderTargets(1, &m_CurrentRTV, TRUE, &m_CurrentDSV);

	if (pCamera) pCamera->SetViewportsAndScissorRects(pd3dCommandList);

	RenderImpl(pd3dCommandList, pCamera);
}

void CScene::RenderImpl(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (m_pd3dGraphicsRootSignature) pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	if (m_pd3dCbvSrvDescriptorHeap)  pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);

	if (pCamera)
	{
		pCamera->SetViewportsAndScissorRects(pd3dCommandList);
		pCamera->UpdateShaderVariables(pd3dCommandList);
	}

	UpdateShaderVariables(pd3dCommandList);

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(2, d3dcbLightsGpuVirtualAddress);

	// Shadow 적용을 유지한다면(메인패스)
	if (m_bEnableShadow && m_pd3dcbShadow) {
		pd3dCommandList->SetGraphicsRootDescriptorTable(17, m_d3dShadowSRV);
		pd3dCommandList->SetGraphicsRootConstantBufferView(18, m_pd3dcbShadow->GetGPUVirtualAddress());
	}
	if (m_pSkyBox)  m_pSkyBox->Render(pd3dCommandList, pCamera);
	if (m_pTerrain) m_pTerrain->Render(pd3dCommandList, pCamera);
	if (m_pMap)     m_pMap->Render(pd3dCommandList, pCamera);
	//if (m_pTerrain) m_pTerrain->Render(pd3dCommandList, pCamera);

	m_CollisionManager.Update(m_pPlayer);
	if (m_bDebugMode)
		m_CollisionManager.RenderDebug(pd3dCommandList, pCamera);
	m_CollisionManager.RenderDamageNumbers(pd3dCommandList, pCamera);

	if (m_pBoss) m_pBoss->Render(pd3dCommandList, pCamera);
	if (m_pGroundAttackRangeEffect) m_pGroundAttackRangeEffect->Render(pd3dCommandList, pCamera);

	for (auto* monster : m_Monsters)
	{
		if (monster) {
			//monster->Animate(m_fElapsedTime);
			monster->Render(pd3dCommandList, pCamera);
		}
	}
	/*	for (int i = 0; i < m_nOtherPlayers; ++i)
			if (m_ppOtherPlayers[i] && m_ppOtherPlayers[i]->visible) m_ppOtherPlayers[i]->Render(pd3dCommandList, pCamera);*/
	for (auto* otherPlayer : m_vPlayers)
		if (otherPlayer && otherPlayer->GetVisible()) otherPlayer->Render(pd3dCommandList, pCamera);

	CTerrainPlayer* pTerrainPlayer = dynamic_cast<CTerrainPlayer*>(m_pPlayer);
	if (pTerrainPlayer)
	{
		if (pTerrainPlayer->m_playerHPBg) pTerrainPlayer->m_playerHPBg->Render(pd3dCommandList, pCamera);
		if (pTerrainPlayer->m_playerHP) pTerrainPlayer->m_playerHP->Render(pd3dCommandList, pCamera);
	}

	for (auto* shader : m_Shaders)
	{
		if (!shader) continue;
		auto* texShader = dynamic_cast<CTextureToScreenShader*>(shader);
		if (texShader && texShader->visible) shader->Render(pd3dCommandList, pCamera);
	}

	for (auto* obj : m_GameObjects)
		if (obj && obj->GetVisible()) obj->Render(pd3dCommandList, pCamera);

	if (m_pFireballSystem) m_pFireballSystem->Render(pd3dCommandList, pCamera);
	if (m_pGreenSpiritSystem) m_pGreenSpiritSystem->Render(pd3dCommandList, pCamera);
	if (m_pHitSparkSystem) m_pHitSparkSystem->Render(pd3dCommandList, pCamera);
	if (m_pDeathBurstSystem) m_pDeathBurstSystem->Render(pd3dCommandList, pCamera);
	if (m_pWeaponThrowSystem) m_pWeaponThrowSystem->Render(pd3dCommandList, pCamera);
	if (m_pBeamSystem) m_pBeamSystem->Render(pd3dCommandList, pCamera);
	if (m_pGroundCrackEffect) m_pGroundCrackEffect->Render(pd3dCommandList, pCamera);
	if (m_pSwordTrailEffect) m_pSwordTrailEffect->Render(pd3dCommandList, pCamera);
}

void CScene::RenderShadowPass(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (!m_pd3dGraphicsRootSignature) return;
	if (!m_pd3dShadowMap || !m_pd3dcbShadow || !m_pcbMappedShadow) return;
	if (!m_pShadowShader || !m_pSkinnedShadowShader) return;
	if (!m_bEnableShadow) return;

	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	if (m_pd3dCbvSrvDescriptorHeap)
		pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);

	{
		XMFLOAT3 lightDir = m_pLights[0].m_xmf3Direction;
		XMFLOAT3 focusPos = (m_pPlayer) ? m_pPlayer->GetPosition() : XMFLOAT3(0, 0, 0);

		XMVECTOR vDir = XMVector3Normalize(XMLoadFloat3(&lightDir));
		XMVECTOR vFocus = XMLoadFloat3(&focusPos);

		float dist = 120.0f;
		XMVECTOR vEye = vFocus - vDir * dist;

		XMMATRIX mLightView = XMMatrixLookAtLH(vEye, vFocus, XMVectorSet(0, 1, 0, 0));
		XMMATRIX mLightProj = XMMatrixOrthographicLH(60.0f, 60.0f, 80.0f, 160.0f);

		XMFLOAT4X4 lv, lp;
		XMStoreFloat4x4(&lv, XMMatrixTranspose(mLightView));
		XMStoreFloat4x4(&lp, XMMatrixTranspose(mLightProj));

		m_pcbMappedShadow->m_xmf4x4LightView = lv;
		m_pcbMappedShadow->m_xmf4x4LightProj = lp;
		m_pcbMappedShadow->m_fShadowBias = 0.0025f;
		m_pcbMappedShadow->m_xmf2ShadowTexel = XMFLOAT2(1.0f / SHADOW_MAP_SIZE, 1.0f / SHADOW_MAP_SIZE);
	}

	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_pd3dShadowMap;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		pd3dCommandList->ResourceBarrier(1, &barrier);
	}

	pd3dCommandList->RSSetViewports(1, &m_ShadowViewport);
	pd3dCommandList->RSSetScissorRects(1, &m_ShadowScissor);

	pd3dCommandList->OMSetRenderTargets(0, nullptr, FALSE, &m_d3dShadowDSV);
	pd3dCommandList->ClearDepthStencilView(m_d3dShadowDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	auto va = m_pd3dcbShadow->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(18, va);

	// Monsters (스키닝이라고 가정)
	for (auto* monster : m_Monsters)
	{
		if (!monster) continue;
		/*m_pSkinnedShadowShader->OnPrepareRender(pd3dCommandList);
		monster->RenderShadow(pd3dCommandList);*/
		monster->RenderShadow(pd3dCommandList, m_pShadowShader, m_pSkinnedShadowShader);
	}

	if (m_pBoss)
	{
/*		m_pSkinnedShadowShader->OnPrepareRender(pd3dCommandList);
		m_pBoss->RenderShadow(pd3dCommandList);*/
		m_pBoss->RenderShadow(pd3dCommandList, m_pShadowShader, m_pSkinnedShadowShader);
	}

	// Player (스키닝이라고 가정)
	if (m_pPlayer)
	{
/*		m_pSkinnedShadowShader->OnPrepareRender(pd3dCommandList);
		m_pPlayer->RenderShadow(pd3dCommandList);*/
		m_pPlayer->RenderShadow(pd3dCommandList, m_pShadowShader, m_pSkinnedShadowShader);
			
	}

	// GameObjects
	for (auto* obj : m_GameObjects)
	{
		if (!obj) continue;
		if (!obj->GetVisible()) continue;

		obj->RenderShadow(pd3dCommandList, m_pShadowShader, m_pSkinnedShadowShader);
	}

	for (auto* otherPlayer : m_vPlayers)
	{
		if (otherPlayer && otherPlayer->GetVisible())
		{
			otherPlayer->RenderShadow(pd3dCommandList, m_pShadowShader, m_pSkinnedShadowShader);
		}
	}

	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_pd3dShadowMap;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		pd3dCommandList->ResourceBarrier(1, &barrier);
	}
}


// ==========================================================================================================
// StartScene
// ==========================================================================================================
void CStartScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, 100);

	m_Shaders.clear();
	m_Shaders.resize(1);

	CTextureToScreenShader* pTextureToScreenShader = new CTextureToScreenShader(1);
	pTextureToScreenShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/StartScene.dds", RESOURCE_TEXTURE2D, 0);
	//pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Font.dds", RESOURCE_TEXTURE2D, 1);

	CreateShaderResourceViews(pd3dDevice, pTexture, 0, 15);

	CScreenRectMeshTextured* pMesh = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, -1.0f, 2.0f, 1.0f, 2.0f);
	pTextureToScreenShader->SetMesh(0, pMesh);
	//pMesh = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, -1.f, 0.12f * 36, -0.5f, 0.20f, 1);
	//pTextureToScreenShader->SetMesh(1, pMesh);
	pTextureToScreenShader->SetTexture(pTexture);

	m_Shaders[0] = pTextureToScreenShader;

	m_GameObjects.clear();
	m_GameObjects.resize(2);

	m_pFontID = new CText(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, L"Enter ID : ", 0.3f, -0.55f);
	m_GameObjects[0] = m_pFontID;

	m_pFontIP = new CText(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, L"Enter IP : ", 0.3f, -0.75f);
	m_GameObjects[1] = m_pFontIP;

	CSoundManager::GetInstance()->PlayBGM("bgm_login");

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CStartScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();

	for (auto* shader : m_Shaders)
	{
		if (!shader) continue;
		shader->ReleaseShaderVariables();
		shader->ReleaseObjects();
		shader->Release();
	}
	m_Shaders.clear();

	for (auto* obj : m_GameObjects)
	{
		if (obj) obj->Release();
	}
	m_GameObjects.clear();
}

void CStartScene::RenderImpl(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (m_pd3dGraphicsRootSignature) pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	if (m_pd3dCbvSrvDescriptorHeap) pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);

	UpdateShaderVariables(pd3dCommandList);

	for (auto* shader : m_Shaders) if (shader) shader->Render(pd3dCommandList, pCamera);
	for (auto* obj : m_GameObjects) if (obj) obj->Render(pd3dCommandList, pCamera);
}

void CStartScene::AnimateObjects(float fTimeElapsed)
{
	if (m_textDirty && m_inputStep == InputStep::EnterID)
	{
		wstring id;
		id.assign(m_inputID.begin(), m_inputID.end());

		m_pFontID->UpdateText(id, L"Enter ID : ");
		m_textDirty = false;
	}
	if (m_textDirty && m_inputStep == InputStep::EnterIP)
	{
		wstring ip;
		ip.assign(m_inputIP.begin(), m_inputIP.end());

		m_pFontIP->UpdateText(ip, L"Enter IP : ");
		m_textDirty = false;
	}
}

void CStartScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_networkInitialized) return;

	switch (nMessageID)
	{
	case WM_KEYDOWN:
		if (m_inputStep == InputStep::EnterID) {
			if (wParam == VK_RETURN) {
				m_inputStep = InputStep::EnterIP;
				::user_name = m_inputID;
			}
			else if (wParam == VK_BACK && !m_inputID.empty()) {
				m_inputID.pop_back();
				m_textDirty = true;
			}
			else if (isprint(wParam) && m_inputID.length() < MAX_ID_LENGTH - 1) {
				m_inputID.push_back((char)wParam);
				m_textDirty = true;
			}

			break;
		}
		if (m_inputStep == InputStep::EnterIP) {
			if (isdigit(wParam) && m_inputIP.length() < 15) {
				m_inputIP.push_back((char)wParam);
				m_textDirty = true;
			}
			else if (wParam == 190) {
				m_inputIP.push_back('.');
				m_textDirty = true;
			}

			else if (wParam == VK_RETURN) {
				m_inputStep = InputStep::Done;
				m_networkInitialized = true;

				char serverIP[16];
				strcpy(serverIP, m_inputIP.c_str());
				std::cout << "Connecting to: " << serverIP << std::endl;
				InitializeNetwork(serverIP); // server IP 전달
				gGameFramework.MoveToNextScene(1);
			}
			else if (wParam == VK_BACK && !m_inputIP.empty()) {
				m_inputIP.pop_back();
			}
		}
		break;
	}
}


// ==========================================================================================================
// SelectScene
// ==========================================================================================================
void CSelectScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CreateCbvSrvDescriptorHeaps(pd3dDevice, 5, 100);

	m_Shaders.clear();
	m_Shaders.resize(2);

	CTextureToScreenShader* pTextureToScreenShader = new CTextureToScreenShader(1);
	pTextureToScreenShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/SelectScene.dds", RESOURCE_TEXTURE2D, 0);

	CreateShaderResourceViews(pd3dDevice, pTexture, 0, 15);

	CScreenRectMeshTextured* pMesh = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, -1.0f, 2.0f, 1.0f, 2.0f);
	pTextureToScreenShader->SetMesh(0, pMesh);
	pTextureToScreenShader->SetTexture(pTexture);

	m_Shaders[0] = pTextureToScreenShader;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CTextureToScreenShader* pTextureToScreenShader1 = new CTextureToScreenShader(1);
	pTextureToScreenShader1->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	CTexture* pTexture1 = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pTexture1->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/loading.dds", RESOURCE_TEXTURE2D, 0);

	CreateShaderResourceViews(pd3dDevice, pTexture1, 1, 15);

	pTextureToScreenShader1->SetMesh(0, pMesh);
	pTextureToScreenShader1->SetTexture(pTexture1);

	m_Shaders[1] = pTextureToScreenShader1;

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CSelectScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();

	for (auto* shader : m_Shaders)
	{
		if (!shader) continue;
		shader->ReleaseShaderVariables();
		shader->ReleaseObjects();
		shader->Release();
	}
	m_Shaders.clear();
}

void CSelectScene::RenderImpl(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (m_pd3dGraphicsRootSignature) pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	if (m_pd3dCbvSrvDescriptorHeap) pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);

	if (pCamera) {
		pCamera->SetViewportsAndScissorRects(pd3dCommandList);
		pCamera->UpdateShaderVariables(pd3dCommandList);
	}

	UpdateShaderVariables(pd3dCommandList);

	const int idx = (loading ? 1 : 0);

	if (idx >= 0 && idx < (int)m_Shaders.size() && m_Shaders[idx])
	{
		m_Shaders[idx]->Render(pd3dCommandList, pCamera);
		if (loading) m_bLoadingRenderedOnce = true;
	}
}

void CSelectScene::AnimateObjects(float fTimeElapsed)
{
	if (m_SceneId != -1 && m_bLoadingRenderedOnce)
	{
		int next = m_SceneId;
		m_SceneId = -1;                 // 중복 전환 방지(권장)
		gGameFramework.RequestMoveToScene(next);
	}
}

void CSelectScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	RECT rt[3] =
	{
		{ 0, 0, FRAME_BUFFER_WIDTH / 3, FRAME_BUFFER_HEIGHT },
		{ FRAME_BUFFER_WIDTH / 3, 0, FRAME_BUFFER_WIDTH / 3 * 2, FRAME_BUFFER_HEIGHT },
		{ FRAME_BUFFER_WIDTH / 3 * 2, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT }
	};

	POINT pt;
	GetCursorPos(&pt);        // screen 좌표
	ScreenToClient(hWnd, &pt); // client 좌표로 변환

	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
		for (int i = 0; i < 3; ++i)
			if (PtInRect(&rt[i], pt)) {
				if (i == 0) gGameFramework.SetSelectedPlayerModel(EPlayerModelType::Knight);
				if (i == 1) gGameFramework.SetSelectedPlayerModel(EPlayerModelType::Wizard);
				if (i == 2) gGameFramework.SetSelectedPlayerModel(EPlayerModelType::Thief);
			}
		break;
	case WM_LBUTTONUP:
	{
		uint8_t job = gGameFramework.GetSelectedJob();
		cs_packet_login p{};
		p.size = sizeof(p);
		p.type = CS_P_LOGIN;
		strcpy_s(p.name, sizeof(p.name), user_name.c_str());
		p.job = job;
		send_packet(&p);

		cout << "[Client] Login Send: Name=" << user_name << " Job=" << static_cast<int>(job) << endl;
	}

	loading = true;
	m_SceneId = 2;
	m_bLoadingRenderedOnce = false;
	break;

	}
}

// ==========================================================================================================
// EndScene
// ==========================================================================================================
void CEndScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, 100);

	m_Shaders.clear();
	m_Shaders.resize(1);

	CTextureToScreenShader* pTextureToScreenShader = new CTextureToScreenShader(1);
	pTextureToScreenShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/EndScene.dds", RESOURCE_TEXTURE2D, 0);
	//pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Font.dds", RESOURCE_TEXTURE2D, 1);

	CreateShaderResourceViews(pd3dDevice, pTexture, 0, 15);

	CScreenRectMeshTextured* pMesh = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, -1.0f, 2.0f, 1.0f, 2.0f);
	pTextureToScreenShader->SetMesh(0, pMesh);
	//pMesh = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, -1.f, 0.12f * 36, -0.5f, 0.20f, 1);
	//pTextureToScreenShader->SetMesh(1, pMesh);
	pTextureToScreenShader->SetTexture(pTexture);

	m_Shaders[0] = pTextureToScreenShader;

	// VICTORY 타이틀 아래에 있던 고정 자막(이미지에 구운 한글 자막, 폰트 미지원으로 네모 깨짐) 대신
	// 플레이어 ID / 클리어 타임을 CText로 표시한다. (다른 씬에서 쓰는 CText와 동일한 방식)
	std::wstring wUserName(user_name.begin(), user_name.end());
	std::wstring idLine = L"ID: " + wUserName;

	float fClearTime = gGameFramework.GetClearTime();
	int nTotalSec = (int)fClearTime;
	int nMin = nTotalSec / 60;
	int nSec = nTotalSec % 60;
	wchar_t szTime[32];
	swprintf_s(szTime, L"time: %02d:%02d", nMin, nSec);

	m_pIDText = new CText(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, idLine, -0.25f, 0.15f);
	m_pTimeText = new CText(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, szTime, -0.25f, -0.05f);

	m_GameObjects.clear();
	m_GameObjects.push_back(m_pIDText);
	m_GameObjects.push_back(m_pTimeText);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CEndScene::UpdateUI(ID3D12GraphicsCommandList* pd3dCommandList)
{
	// 기본 CScene::UpdateUI()는 m_GameObjects에 있는 CText를 전부
	// "LV. " + m_pPlayer->level[...] 로 덮어쓴다. 이건 CTerrainPlayer(실제 게임 플레이 씬)의
	// 레벨 표시 텍스트를 위한 로직인데, EndScene은 그런 텍스트가 없고 대신
	// m_pIDText/m_pTimeText를 갖고 있어서, 기본 동작을 그대로 물려받으면
	// 이 둘이 매 프레임 "LV. 1"(EndScene의 CPlayer는 레벨을 세팅한 적이 없어 기본값 1)로
	// 덮어써져 버린다. EndScene에서는 이 로직 자체가 필요 없으므로 아무것도 하지 않는다.
}

void CEndScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();

	for (auto* shader : m_Shaders)
	{
		if (!shader) continue;
		shader->ReleaseShaderVariables();
		shader->ReleaseObjects();
		shader->Release();
	}
	m_Shaders.clear();

	for (auto* obj : m_GameObjects)
	{
		if (obj) obj->Release();
	}
	m_GameObjects.clear();
	m_pIDText = nullptr;
	m_pTimeText = nullptr;
}

void CEndScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (m_pd3dGraphicsRootSignature) pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	if (m_pd3dCbvSrvDescriptorHeap) pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);

	if (pCamera) {
		pCamera->SetViewportsAndScissorRects(pd3dCommandList);
		pCamera->UpdateShaderVariables(pd3dCommandList);
	}
	UpdateShaderVariables(pd3dCommandList);

	for (auto* shader : m_Shaders) if (shader) shader->Render(pd3dCommandList, pCamera);
	for (auto* obj : m_GameObjects) if (obj) obj->Render(pd3dCommandList, pCamera);
}
