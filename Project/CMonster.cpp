#include "stdafx.h"
#include "CMonster.h"
#include "Player.h"
#include "Network.h"   // g_monsters
#include "SoundManager.h"
#include "CDeathBurstSystem.h"

CDeathBurstSystem* CMonster::s_pDeathBurstSystem = nullptr;

CMonster::CMonster(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
    const char* pstrModelPath, int nAnimationTracks, CLoadedModelInfo* pModel, float fMaxHP, int id)
    : CGameObject(1), m_fMaxHP(fMaxHP), m_fMonsterHP(fMaxHP)
{
    CLoadedModelInfo* pMonsterModel = pModel;
    if (!pMonsterModel)
        pMonsterModel = CGameObject::LoadGeometryAndAnimationFromFile(
            pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pstrModelPath, NULL);

    SetChild(pMonsterModel->m_pModelRootObject, true);

    m_pSkinnedAnimationController = new CAnimationController(
        pd3dDevice, pd3dCommandList, nAnimationTracks, pMonsterModel);

    // 0=Idle, 1=Walk, 2=Attack, 3=GetHit, 4=Death
    for (int i = 0; i < nAnimationTracks; ++i)
        m_pSkinnedAnimationController->SetTrackAnimationSet(i, i);

    m_pSkinnedAnimationController->SetTrackType(3, ANIMATION_TYPE_ONCE); // GetHit
    m_pSkinnedAnimationController->SetTrackType(4, ANIMATION_TYPE_ONCE); // Death

    // Idle만 활성화
    for (int i = 1; i < nAnimationTracks; ++i)
        m_pSkinnedAnimationController->SetTrackEnable(i, false);

    SetScale(1.0f, 1.0f, 1.0f);

    m_pHpbar = new Hpbar(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);

    SetMonsterID(id);
    {
        std::lock_guard<std::mutex> lock(g_monster_mutex);
        g_monsters[id] = this;
    }

    CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CMonster::~CMonster()
{
    // g_monsters에서 자신을 제거하지 않으면, 씬 전환으로 이 몬스터가 delete된 뒤에도
    // g_monsters엔 죽은 포인터가 그대로 남아있게 된다. 그 상태에서 AnimateObjects()의
    // 순회나 뒤늦게 도착한 네트워크 패킷(UpdateMonsterPosition 등)이 그 포인터를 쓰면
    // use-after-free로 크래시가 난다.
    {
        std::lock_guard<std::mutex> lock(g_monster_mutex);
        auto it = g_monsters.find(m_nMonsterID);
        if (it != g_monsters.end() && it->second == this)
        {
            g_monsters.erase(it);
        }
    }

    if (m_pHpbar) delete m_pHpbar;
}

// -----------------------------------------------------------------------

void CMonster::TakeDamage(float damage)
{
    if (m_eState == MonsterState::Death) return;

    m_fMonsterHP = max(0.0f, m_fMonsterHP - damage);
    m_fHpRatio = m_fMonsterHP / m_fMaxHP;

    send_hit_damage(m_nMonsterID, (int)damage);

    if (m_fMonsterHP <= 0.0f)
    {
        int randomIndex = (rand() % 2) + 1;
        string sfxName = "monster_die_" + to_string(randomIndex);
        CSoundManager::GetInstance()->PlaySFX(sfxName);
        if (s_pDeathBurstSystem) s_pDeathBurstSystem->Emit(GetPosition());
        TransitionTo(MonsterState::Death);
    }
    else
    {
        int randomIndex = (rand() % 2) + 1;
        string sfxName = "monster_hurt_" + to_string(randomIndex);
        CSoundManager::GetInstance()->PlaySFX(sfxName);
        TransitionTo(MonsterState::GetHit);
    }
}

void CMonster::TransitionTo(MonsterState newState)
{
    if (m_eState == MonsterState::Death) return;
    if (!m_pSkinnedAnimationController)  return;

    auto toTrack = [](MonsterState s) -> int {
        switch (s) {
        case MonsterState::Idle:   return 0;
        case MonsterState::Walk:   return 1;
        case MonsterState::Attack: return 2;
        case MonsterState::GetHit: return 3;
        case MonsterState::Death:  return 4;
        default:                   return 0;
        }
        };

    m_pSkinnedAnimationController->SetTrackEnable(toTrack(m_eState), false);
    m_eState = newState;

    int newTrack = toTrack(m_eState);
    m_pSkinnedAnimationController->SetTrackPosition(newTrack, 0.0f);
    m_pSkinnedAnimationController->SetTrackEnable(newTrack, true);
}

void CMonster::Animate(float fTimeElapsed)
{
    CGameObject::Animate(fTimeElapsed);
}

void CMonster::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    CGameObject::Render(pd3dCommandList, pCamera);
    if (m_pHpbar && !IsDead())
    {
        XMFLOAT3 pos = GetPosition();

        m_pHpbar->SetPosition(pos.x, pos.y + 2.5f, pos.z);
        m_pHpbar->SetHpRatio(m_fHpRatio);
        m_pHpbar->Render(pd3dCommandList, pCamera);
    }
}