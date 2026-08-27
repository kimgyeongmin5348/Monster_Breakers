#pragma once
#include "Object.h"

class CGroundAttackRangeEffect
{
public:
    CGroundAttackRangeEffect();
    ~CGroundAttackRangeEffect();

    void Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
        ID3D12RootSignature* pd3dGraphicsRootSignature, int nPoolSize = 4);
    void Release();
    void ReleaseUploadBuffers();
    void SetTerrain(CHeightMapTerrain* pTerrain) { m_pTerrain = pTerrain; }

    // xmf3Center   : 바닥 중심 위치(보통 공격자의 현재 위치, Y는 지면 높이)
    // fRadius      : 공격범위 반지름
    // fWarmupTime  : 경고가 표시된 후 실제로 타격되기까지 걸리는 시간(초). 이 시간 동안 원이 가운데서부터 차오름
    // xmf4Color    : 경고 색상(RGB). A는 내부적으로 페이드용으로 덮어쓰므로 의미 없음
    void Spawn(const XMFLOAT3& xmf3Center, float fRadius, float fWarmupTime,
        const XMFLOAT4& xmf4Color = XMFLOAT4(1.0f, 0.15f, 0.05f, 1.0f));

    // 부채꼴(콘) 범위 버전 - SWEEP 등 방향성 공격
    // xmf3Direction : 부채꼴이 향하는 방향(보통 공격자의 m_look, 월드 XZ 평면 벡터, Y는 무시됨)
    // fHalfAngleDeg : 부채꼴의 "절반" 각도(도). 예) 전방 120도 콘이면 60을 전달
    void Spawn(const XMFLOAT3& xmf3Center, float fRadius, float fWarmupTime,
        const XMFLOAT3& xmf3Direction, float fHalfAngleDeg,
        const XMFLOAT4& xmf4Color = XMFLOAT4(1.0f, 0.15f, 0.05f, 1.0f));

    void Animate(float fTimeElapsed);
    void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

private:
    struct Indicator
    {
        CGameObject* pObject = nullptr;
        bool         bActive = false;
        float        fTimer = 0.0f;
        float        fWarmupTime = 1.0f;
        float        fFadeOutTime = 0.18f; // 타격 순간 이후 사라지는 시간
        XMFLOAT3     xmf3Color = XMFLOAT3(1.0f, 0.15f, 0.05f);

        // 부채꼴(콘) 표시용 - bSector=false면 그냥 원형(기존 동작)
        bool         bSector = false;
        float        fFacingAngle = 0.0f;   // 메쉬 로컬 좌표계 기준 정면 각도(라디안)
        float        fHalfAngle = XM_PI;    // 부채꼴 절반각(라디안). 원형일 땐 의미 없음
    };

    void PlaceOnGround(CGameObject* pObject, const XMFLOAT3& xmf3Center, float fRadius);
    int  AcquireSlot(); // 비활성 슬롯 또는 라운드로빈으로 재사용할 인덱스 선택

    // 월드 XZ 방향 벡터를 PSGroundRange가 쓰는 "메쉬 로컬 평면" 각도로 변환.
    // PlaceFlatOnGround의 RotationX(90도) 매핑과 한 곳에서만 맞춰주면 됨.
    static float WorldDirectionToLocalAngle(const XMFLOAT3& xmf3Direction);

    std::vector<Indicator> m_vIndicators;
    int                     m_nNextIndex = 0; // 풀이 가득 찼을 때 다음에 재사용할 인덱스(라운드로빈)
    CHeightMapTerrain*      m_pTerrain = nullptr;
};
