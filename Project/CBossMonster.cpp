#include "stdafx.h"
#include "CBossMonster.h"
#include "Player.h"
#include "Network.h"   // g_monsters / send_hit_damage
#include "SoundManager.h"
#include "CDeathBurstSystem.h"

namespace
{
    constexpr int TRACK_IDLE = 0;
    constexpr int TRACK_WALK = 1;
    constexpr int TRACK_ATTACK01 = 2;
    constexpr int TRACK_ATTACK02 = 3;
    constexpr int TRACK_TAUNT = 4;
    constexpr int TRACK_DEATH = 5;
    constexpr int BOSS_ANIMATION_TRACKS = 6;

    // 보스 hpbar 화면 위치/크기 (화면 상단 우측, 플레이어 hpbar 바로 위).
    // 좌표계는 플레이어 hpbar와 동일: left는 시작 x, 거기서 MAX_WIDTH만큼 우측으로 뻗는다.
    constexpr float BOSS_HPBAR_LEFT = -0.5f;
    constexpr float BOSS_HPBAR_MAX_WIDTH = 1.0f;
    constexpr float BOSS_HPBAR_TOP = 0.7f;
    constexpr float BOSS_HPBAR_HEIGHT = 0.01f;
}

CBossMonster::CBossMonster(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
    const char* pstrModelPath, CLoadedModelInfo* pModel, float fMaxHP, int id)
    : CGameObject(1), m_fMaxHP(fMaxHP), m_fMonsterHP(fMaxHP)
{
    CLoadedModelInfo* pBossModel = pModel;
    if (!pBossModel)
        pBossModel = CGameObject::LoadGeometryAndAnimationFromFile(
            pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pstrModelPath, NULL);

    SetChild(pBossModel->m_pModelRootObject, true);

    m_pSkinnedAnimationController = new CAnimationController(
        pd3dDevice, pd3dCommandList, BOSS_ANIMATION_TRACKS, pBossModel);

    // 0=Idle, 1=Walk, 2=Attack01, 3=Attack02, 4=Taunt, 5=Death
    for (int i = 0; i < BOSS_ANIMATION_TRACKS; ++i)
        m_pSkinnedAnimationController->SetTrackAnimationSet(i, i);

    m_pSkinnedAnimationController->SetTrackType(TRACK_ATTACK01, ANIMATION_TYPE_ONCE);
    m_pSkinnedAnimationController->SetTrackType(TRACK_ATTACK02, ANIMATION_TYPE_ONCE);
    m_pSkinnedAnimationController->SetTrackType(TRACK_TAUNT, ANIMATION_TYPE_ONCE);
    m_pSkinnedAnimationController->SetTrackType(TRACK_DEATH, ANIMATION_TYPE_ONCE);

    // Idle만 활성화
    for (int i = 1; i < BOSS_ANIMATION_TRACKS; ++i)
        m_pSkinnedAnimationController->SetTrackEnable(i, false);

    // 플레이어 hpbar(SetHPWidth)와 동일한 (left, width, top, height) 시그니처를 사용.
    // 사진 기준 화면 상단 우측에 배치: 플레이어 hpbar(top=0.85f)보다 위쪽, 약간 더 큰 폭.
    m_pBossHpbar = new CScreenShader(1);
    m_pBossHpbar->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
    CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
    pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/hp.dds", RESOURCE_TEXTURE2D, 0);
    CScene::CreateShaderResourceViews(pd3dDevice, pTexture, 0, 15);
    CScreenRectMeshTextured* pMesh = new CScreenRectMeshTextured(pd3dDevice, pd3dCommandList, BOSS_HPBAR_LEFT, BOSS_HPBAR_MAX_WIDTH, BOSS_HPBAR_TOP, BOSS_HPBAR_HEIGHT);
    m_pBossHpbar->SetMesh(0, pMesh);
    m_pBossHpbar->SetTexture(pTexture);

    // SetHPWidth에서 mesh를 다시 만들 때 필요
    m_pd3dDevice = pd3dDevice;
    m_pd3dCommandList = pd3dCommandList;

    SetMonsterID(id);
    // g_monsters[id] = this;

    CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CBossMonster::~CBossMonster()
{}

void CBossMonster::SetVisualScale(float fScale)
{
    m_fVisualScale = (std::max)(fScale, 0.01f);
    ApplyVisualScale();
}

void CBossMonster::SetTerrain(CHeightMapTerrain* pTerrain)
{
    m_pTerrain = pTerrain;
    if (m_pGroundAttackRangeEffect)
        m_pGroundAttackRangeEffect->SetTerrain(m_pTerrain);
}

void CBossMonster::SetLookDirection(const XMFLOAT3& xmf3Look)
{
    XMFLOAT3 pos = GetPosition();
    XMFLOAT3 target(pos.x + xmf3Look.x, pos.y + xmf3Look.y, pos.z + xmf3Look.z);
    XMFLOAT3 up(0.0f, 1.0f, 0.0f);
    LookAt(target, up);
    ApplyVisualScale();
}

void CBossMonster::SetPositionOnTerrain(const XMFLOAT3& xmf3ServerPosition)
{
    XMFLOAT3 position = xmf3ServerPosition;

    if (m_pTerrain)
    {
        // CTerrainPlayer와 같은 HeightMap 원점/보간 규칙을 사용한다.
        constexpr float TERRAIN_WORLD_X = -156.71f;
        constexpr float TERRAIN_WORLD_Y = -14.43f;
        constexpr float TERRAIN_WORLD_Z = -255.0f;

        const XMFLOAT3 terrainScale = m_pTerrain->GetScale();
        const float localX = position.x - TERRAIN_WORLD_X;
        const float localZ = position.z - TERRAIN_WORLD_Z;
        const float terrainWidth = (m_pTerrain->GetHeightMapWidth() - 1) * terrainScale.x;
        const float terrainLength = (m_pTerrain->GetHeightMapLength() - 1) * terrainScale.z;

        // 맵 바깥 위치(스폰 전의 숨김 좌표 등)는 원래 서버 Y를 유지한다.
        if (localX >= 0.0f && localZ >= 0.0f && localX < terrainWidth && localZ < terrainLength)
        {
            const int z = static_cast<int>(localZ / terrainScale.z);
            const bool bReverseQuad = (z % 2) != 0;
            position.y = m_pTerrain->GetHeight(localX, localZ, bReverseQuad) + TERRAIN_WORLD_Y;
        }
    }

    CGameObject::SetPosition(position);
}

void CBossMonster::ApplyVisualScale()
{
    // GetRight/Up/Look은 정규화한 축을 돌려준다. 따라서 LookAt으로 방향이
    // 갱신된 뒤에도 여기서 축에만 크기를 곱하면 위치를 건드리지 않고 크기가 유지된다.
    const XMFLOAT3 right = GetRight();
    const XMFLOAT3 up = GetUp();
    const XMFLOAT3 look = GetLook();

    m_xmf4x4ToParent._11 = right.x * m_fVisualScale;
    m_xmf4x4ToParent._12 = right.y * m_fVisualScale;
    m_xmf4x4ToParent._13 = right.z * m_fVisualScale;
    m_xmf4x4ToParent._21 = up.x * m_fVisualScale;
    m_xmf4x4ToParent._22 = up.y * m_fVisualScale;
    m_xmf4x4ToParent._23 = up.z * m_fVisualScale;
    m_xmf4x4ToParent._31 = look.x * m_fVisualScale;
    m_xmf4x4ToParent._32 = look.y * m_fVisualScale;
    m_xmf4x4ToParent._33 = look.z * m_fVisualScale;

    UpdateTransform(NULL);
}

void CBossMonster::SetGroundAttackRangeEffect(CGroundAttackRangeEffect* pEffect)
{
    m_pGroundAttackRangeEffect = pEffect;
    if (m_pGroundAttackRangeEffect)
        m_pGroundAttackRangeEffect->SetTerrain(m_pTerrain);
}

int CBossMonster::TrackOf(BossState s) const
{
    switch (s)
    {
    case BossState::Idle:     return TRACK_IDLE;
    case BossState::Walk:     return TRACK_WALK;
    case BossState::Attack01: return TRACK_ATTACK01;
    case BossState::Attack02: return TRACK_ATTACK02;
    case BossState::Taunt:    return TRACK_TAUNT;
    case BossState::Death:    return TRACK_DEATH;
    default:                  return TRACK_IDLE;
    }
}

// newState(애니메이션 트랙)에 따라 공격범위 이펙트의 모양/색상/웜업을 결정해서 스폰한다.
// - Attack01(NORMAL) : 원형, 노랑, 웜업 0.3초
// - Attack02(SLAM)    : 원형, 빨강, 웜업 0.6초
// - Taunt(SWEEP)      : 부채꼴(보스가 보는 방향 기준), 주황, 웜업 0.6초
// - Idle/Walk/Death   : 이펙트 없음
void CBossMonster::SpawnAttackEffectFor(BossState newState, const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Look,
    float fRadius, float fSweepAngleDeg)
{
    if (!m_pGroundAttackRangeEffect) return;

    switch (newState)
    {
    case BossState::Attack01:
    {
        XMFLOAT4 color(1.0f, 0.8f, 0.0f, 1.0f); // 노랑
        m_pGroundAttackRangeEffect->Spawn(xmf3Center, fRadius, 0.3f, color);
        break;
    }
    case BossState::Attack02:
    {
        XMFLOAT4 color(1.0f, 0.1f, 0.05f, 1.0f); // 빨강
        m_pGroundAttackRangeEffect->Spawn(xmf3Center, fRadius, 0.6f, color);
        break;
    }
    case BossState::Taunt:
    {
        XMFLOAT4 color(1.0f, 0.5f, 0.0f, 1.0f); // 주황
        float halfAngleDeg = fSweepAngleDeg * 0.5f;
        // 이동 패킷과 패턴 패킷의 도착 순서가 달라도 방향이 틀어지지 않도록
        // 현재 모델 행렬이 아닌, 이 공격을 판정한 서버 look을 그대로 쓴다.
        m_pGroundAttackRangeEffect->Spawn(xmf3Center, fRadius, 0.6f, xmf3Look, halfAngleDeg, color);
        break;
    }
    default:
        // Idle/Walk/Death 등은 공격범위 이펙트 없음
        break;
    }
}

void CBossMonster::TransitionTo(BossState newState)
{
    if (m_eState == BossState::Death) return;
    if (!m_pSkinnedAnimationController)  return;

    auto toTrack = [](BossState s) -> int {
        switch (s) {
        case BossState::Idle:   return 0;
        case BossState::Walk:   return 1;
        case BossState::Attack01: return 2;
        case BossState::Attack02: return 3;
        case BossState::Taunt:  return 4;
        case BossState::Death:  return 5;
        default:                   return 0;
        }
        };

    m_pSkinnedAnimationController->SetTrackEnable(toTrack(m_eState), false);
    m_eState = newState;

    int newTrack = toTrack(m_eState);
    m_pSkinnedAnimationController->SetTrackPosition(newTrack, 0.0f);
    m_pSkinnedAnimationController->SetTrackEnable(newTrack, true);
}

void CBossMonster::PlayAttackPattern(BossState newState, const XMFLOAT3& xmf3Center, const XMFLOAT3& look, float fRadius, float fSweepAngleDeg)
{
    if (m_eState == BossState::Death) return; // 죽은 보스는 패턴 패킷이 와도 무시(이펙트도 스폰하지 않음)

    switch (newState)
    {
    case BossState::Attack01:
        CSoundManager::GetInstance()->PlaySFX("boss_attack_1");
        break;
    case BossState::Attack02:
        CSoundManager::GetInstance()->PlaySFX("boss_attack_2");
        break;
    case BossState::Taunt:
        CSoundManager::GetInstance()->PlaySFX("boss_attack_3");
        break;
    }

    TransitionTo(newState);
    SpawnAttackEffectFor(newState, xmf3Center, look, fRadius, fSweepAngleDeg);
}

void CBossMonster::TakeDamage(float damage)
{
    if (m_eState == BossState::Death) return;

    m_fMonsterHP = max(0.0f, m_fMonsterHP - damage);
    m_fHpRatio = m_fMonsterHP / m_fMaxHP;

    send_hit_damage(m_nMonsterID, (int)damage);

    if (m_fMonsterHP <= 0.0f)
    {
        CSoundManager::GetInstance()->PlaySFX("boss_die_1");
        if (m_pDeathBurstSystem) m_pDeathBurstSystem->Emit(GetPosition());
        TransitionTo(BossState::Death);
    }
    else
    {
        CSoundManager::GetInstance()->PlaySFX("boss_hurt");
    }
}

void CBossMonster::Animate(float fTimeElapsed)
{
    CGameObject::Animate(fTimeElapsed);
    ApplyVisualScale();
}

void CBossMonster::Update(float fTimeElapsed)
{
    // 플레이어(CTerrainPlayer::Update)의 SetHPWidth 호출 방식과 동일하게,
    // HP 비율이 바뀔 때만 mesh를 다시 만들어 교체한다.
    float newWidth = m_fHpRatio * BOSS_HPBAR_MAX_WIDTH;

    if (fabs(m_fPrevHpbarWidth - newWidth) > 0.001f)
    {
        m_fPrevHpbarWidth = newWidth;
        SetHPWidth(newWidth);
    }

    if (m_eState == BossState::Walk)
    {
        m_fWalkSoundTimer += fTimeElapsed;

        if (m_fWalkSoundTimer >= 0.8f)
        {
            CSoundManager::GetInstance()->PlaySFX("boss_walk");
            m_fWalkSoundTimer = 0.0f;
        }
    }
    else
    {
        m_fWalkSoundTimer = 0.0f;
    }
}

void CBossMonster::SetHPWidth(float newWidth)
{
    if (!m_pBossHpbar) return;

    // 더 이상 메시를 새로 만들고 SetMesh로 교체하지 않는다.
    // (예전 방식은 폭이 바뀔 때마다 GPU 버텍스 버퍼를 즉시 해제했는데,
    //  더블 버퍼링 때문에 GPU가 이전 프레임에서 그 버퍼를 아직 읽는 중일 수 있어
    //  힙 손상으로 이어질 수 있었다. CScreenRectMeshTextured::UpdateRect()는
    //  이미 매핑된 같은 버퍼의 내용만 갱신하므로 이 문제가 없다.)
    if (m_pBossHpbar->m_nMeshes > 0 && m_pBossHpbar->m_ppMeshes[0])
    {
        auto* pRect = static_cast<CScreenRectMeshTextured*>(m_pBossHpbar->m_ppMeshes[0]);
        pRect->UpdateRect(BOSS_HPBAR_LEFT, newWidth, BOSS_HPBAR_TOP, BOSS_HPBAR_HEIGHT);
    }
}

void CBossMonster::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    CGameObject::Render(pd3dCommandList, pCamera);

    if (m_pBossHpbar && m_bHpbarVisible && m_eState != BossState::Death)
        m_pBossHpbar->Render(pd3dCommandList, pCamera);
}
