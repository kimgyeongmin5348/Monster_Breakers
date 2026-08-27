#pragma once

#include "Object.h"
#include "Common.h"

class OtherPlayer : public CGameObject
{
public:
	//void SetPosition(const XMFLOAT3& position);
	//void SetMovement(bool isMoving);
	OtherPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel);
	virtual ~OtherPlayer();
    
    float	currentHP = 100.f;
    float	maxHP = 100.f;
    long long networkID = -1;
    std::wstring playerID;
	float m_fHitFlashTimer = 0.0f;
	static constexpr float HIT_FLASH_DURATION = 0.4f;
	static constexpr float HIT_FLASH_BLINK_SPEED = 25.0f;

    int     level[3] = { 1,1,1 };
	float damage = 1.f;

    int currentAnim = 0; // 현재 재생 애니메이션
    int targetAnim = 0; // 서버에서 받은 애니메이션

    XMFLOAT3					m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    XMFLOAT3					m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
    XMFLOAT3					m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
    XMFLOAT3					m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);
    
    XMFLOAT3                    m_xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

    float           			m_fPitch = 0.0f;
    float           			m_fYaw = 0.0f;
    float           			m_fRoll = 0.0f;

    bool isConnedted = false;

public:
	virtual void Animate(int animation, float fTimeElapsed);

    void Rotate(XMFLOAT3 look, XMFLOAT3 right)
    {
        XMVECTOR lookVector = XMLoadFloat3(&look);
        XMVECTOR rightVector = XMLoadFloat3(&right);
        XMVECTOR upVector = XMVector3Cross(lookVector, rightVector);

        lookVector = XMVector3Normalize(lookVector);
        rightVector = XMVector3Normalize(rightVector);
        upVector = XMVector3Normalize(upVector);

        XMStoreFloat3(&m_xmf3Look, lookVector);
        XMStoreFloat3(&m_xmf3Right, rightVector);
        XMStoreFloat3(&m_xmf3Up, upVector);
    }

    virtual void OnPrepareRender();
    void PlayAnimationTrack(int trackIndex, float speed = 1.0f);
    bool IsAnimationFinished(int trackIndex);

    AnimationBlend m_animBlend;

    void StartAnimationBlend(int fromTrack, int toTrack, float blendTime);
	void TriggerHitFlash() { m_fHitFlashTimer = HIT_FLASH_DURATION; }

};
