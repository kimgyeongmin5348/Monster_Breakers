#pragma once
#include "Object.h"
#include "Hpbar.h"

class CPlayer;

enum class MonsterState
{
    Idle,
    Walk,
    Attack,
    GetHit,
    Death
};

class CMonster : public CGameObject
{
public:
    CMonster(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
        ID3D12RootSignature* pd3dGraphicsRootSignature,
        const char* pstrModelPath, int nAnimationTracks,
        CLoadedModelInfo* pModel = nullptr, float fMaxHP = 100.0f, long long id = -1);
    virtual ~CMonster();

    virtual void Animate(float fTimeElapsed) override;
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL) override;

    void         TransitionTo(MonsterState newState);
    MonsterState GetState() const { return m_eState; }

    void  TakeDamage(float damage);
    float GetHP()      const { return m_fMonsterHP; }
    float GetHPRatio() const { return m_fHpRatio; }
    bool  IsDead()     const { return m_fMonsterHP <= 0.0f; }

    void SetPlayer(CPlayer* p) { m_pPlayer = p; }
    void SetMonsterID(long long id) { m_nMonsterID = id; }
    long long GetMonsterID() const { return m_nMonsterID; }

    // Scene에서 한 줄로 특정 몬스터 종류를 N마리 생성
    // startID부터 순서대로 ID 할당, g_monsters에도 등록
    // 반환: 생성된 CMonster* 벡터 (크기 = count)
/*    static std::vector<CMonster*> SpawnGroup(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
        ID3D12RootSignature* pd3dGraphicsRootSignature, const char* pstrModelPath,   // "Model/Monster/BattleBeePA.bin" 등
        int                        count,            // 몇 마리 생성할지
        int                        startID,          // 서버 ID 시작값
        float                      fMaxHP = 100.0f,
        float                      fScale = 1.0f);*/

    Hpbar* m_pHpbar = nullptr;


    void ResetHP() {
        m_fMonsterHP = m_fMaxHP;
        m_fHpRatio = 1.0f;
        m_eState = MonsterState::Idle;  // Death 상태 해제

        // 모든 트랙 끄고 Idle만 켜기
        if (m_pSkinnedAnimationController) {
            for (int i = 1; i <= 4; ++i)
                m_pSkinnedAnimationController->SetTrackEnable(i, false);
            m_pSkinnedAnimationController->SetTrackPosition(0, 0.0f);
            m_pSkinnedAnimationController->SetTrackEnable(0, true);
        }
    }

    void SetHP(float hp) {
        m_fMonsterHP = hp;
        m_fHpRatio = m_fMonsterHP / m_fMaxHP;
    }

private:
    CPlayer* m_pPlayer = nullptr;
    long long    m_nMonsterID = -1;

    float        m_fMonsterHP = 100.0f;
    float        m_fMaxHP = 1000.0f;
    float        m_fHpRatio = 1.0f;

    MonsterState m_eState = MonsterState::Idle;

};
