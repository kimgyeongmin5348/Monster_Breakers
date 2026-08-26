#pragma once

#include <vector>
#include "QuadTree.h"
#include "CFireballSystem.h"
#include "CWeaponThrowSystem.h"
#include "CDebugShader.h"
#include "CCubeMesh.h"
#include "DamageNumber.h"
#include "CHitSparkSystem.h"

class CGameObject;
class CPlayer;
class CMonster;
class CBossMonster;

class CCollisionManager
{
private:
    CQuadTree* m_pQuadTree = NULL;
    std::vector<CGameObject*> m_objects;
    std::vector<ColliderInfo> m_colliderinfos;
    std::vector<CMonster*>* m_pMonsters = nullptr;
    CBossMonster* m_pBoss = nullptr; // 보스는 목록이 아니라 단일 포인터로 별도 관리

    CFireballSystem* m_pFireballSystem = nullptr;
    CWeaponThrowSystem* m_pWeaponThrowSystem = nullptr;
    CHitSparkSystem* m_pHitSparkSystem = nullptr;

    int frameCounter = 0;
    bool m_bHitProcessed = false;     // 일반 몬스터용 (기존 로직, 건드리지 않음)
    bool m_bBossHitProcessed = false; // 보스용 (한 스윙에 한 번만 데미지 들어가게)

    CGameObject* m_pDebugCube;
    CGameObject* m_pDebugSphere;

    CDamageNumberSystem* m_pDamageNumberSystem = nullptr;

public:
    CCollisionManager();
    ~CCollisionManager();

    void Build(const BoundingBox& worldBounds, int maxObjectsPerNode, int maxDepth);

    void InsertObject(CGameObject* object);
    void InsertCollider(const ColliderInfo& collider);

    void PrintTree();

    bool CheckIntersection(const BoundingBox& bounds, const ColliderInfo& col);

    void SetMonsters(std::vector<CMonster*>* monsters) { m_pMonsters = monsters; }
    void SetBoss(CBossMonster* pBoss) { m_pBoss = pBoss; }
    void SetFireballSystem(CFireballSystem* p) { m_pFireballSystem = p; }
    void SetWeaponThrowSystem(CWeaponThrowSystem* p) { m_pWeaponThrowSystem = p; }
    void SetHitSparkSystem(CHitSparkSystem* p) { m_pHitSparkSystem = p; }

    void Update(CPlayer* player);

    bool IsColliding(const BoundingBox& box1, const BoundingBox& box2);

    void InitializeDebugObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
    void RenderDebug(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

    // 데미지 숫자 파티클: Scene 초기화 시 한 번 호출하고,
    // 매 프레임 렌더 루프에서 RenderDamageNumbers()를 호출해주면 된다.
    // Update(CPlayer*) 내부에서 데미지가 들어갈 때마다 자동으로 Spawn되고,
    // 시스템 자체의 애니메이션(상승/페이드)도 Update(CPlayer*) 안에서 매 프레임 갱신된다.
    void InitializeDamageNumberSystem(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
    void RenderDamageNumbers(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
    // Update(CPlayer*)에는 fTimeElapsed가 없어서 별도로 노출한다.
    // 메인 루프에서 CCollisionManager::Update(player)를 호출하는 바로 그 자리에서
    // fTimeElapsed와 함께 이것도 한 번 호출해주면 된다.
    void UpdateDamageNumbers(float fTimeElapsed);

private:
    void CollectNearbyObjects(QuadTreeNode* node, const BoundingBox& aabb,
        std::vector<CGameObject*>& outDynamics,
        std::vector<ColliderInfo>& outStatics);
    void HandleCollision(CPlayer* player, CGameObject* obj);
    void HandleCollision(CPlayer* player, const ColliderInfo& colinfo);
};