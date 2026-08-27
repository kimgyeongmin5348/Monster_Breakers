#include "stdafx.h"
#include "OtherPlayer.h"

OtherPlayer::OtherPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel)
{
	CLoadedModelInfo* pPlayerModel = pModel;
	if (!pPlayerModel) pPlayerModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/Knight.bin", NULL);

	SetChild(pPlayerModel->m_pModelRootObject, true);

	m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 7, pPlayerModel);
	m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0); // ±âº»
	m_pSkinnedAnimationController->SetTrackAnimationSet(1, 1); // °È±â
	m_pSkinnedAnimationController->SetTrackAnimationSet(2, 2); // ¶Ù±â
	m_pSkinnedAnimationController->SetTrackAnimationSet(3, 3); // ±âº»°ø°Ý
	m_pSkinnedAnimationController->SetTrackAnimationSet(4, 4); // skill 1
	m_pSkinnedAnimationController->SetTrackAnimationSet(5, 5); // skill 2
	m_pSkinnedAnimationController->SetTrackAnimationSet(6, 6); // skill 3

	m_pSkinnedAnimationController->SetTrackType(3, ANIMATION_TYPE_ONCE);
	m_pSkinnedAnimationController->SetTrackType(4, ANIMATION_TYPE_ONCE);
	m_pSkinnedAnimationController->SetTrackType(5, ANIMATION_TYPE_ONCE);
	m_pSkinnedAnimationController->SetTrackType(6, ANIMATION_TYPE_ONCE);
	m_pSkinnedAnimationController->SetTrackSpeed(4, 1.5);
	m_pSkinnedAnimationController->SetTrackSpeed(5, 1.5);
	m_pSkinnedAnimationController->SetTrackSpeed(6, 1.5);

	SetPosition(-1000, -1000, -1000);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	//if (pPlayerModel) delete pPlayerModel;
}

OtherPlayer::~OtherPlayer()
{
}

//void OtherPlayer::SetPosition(const XMFLOAT3& position) {
//	m_xmf3Position = position;
//	UpdateTransform();
//}

void OtherPlayer::Animate(int animation, float fTimeElapsed)
{
	if (m_pSkinnedAnimationController)
	{
		if (m_animBlend.active)
		{
			m_animBlend.elapsed += fTimeElapsed;
			float t = m_animBlend.elapsed / m_animBlend.duration;
			if (t >= 1.0f)
			{
				t = 1.0f;
				m_animBlend.active = false;
				for (int i = 0; i < 7; ++i)
					m_pSkinnedAnimationController->SetTrackEnable(i, i == m_animBlend.to);

				m_pSkinnedAnimationController->SetTrackWeight(m_animBlend.to, 1.0f);
				m_pSkinnedAnimationController->SetTrackWeight(m_animBlend.from, 0.0f);
			
				m_pSkinnedAnimationController->SetTrackPosition(m_animBlend.from, 0.0f);
			}
			else
			{
				m_pSkinnedAnimationController->SetTrackWeight(m_animBlend.from, 1.0f - t);
				m_pSkinnedAnimationController->SetTrackWeight(m_animBlend.to, t);
			}
		}
		switch (animation)
		{
		case 3:
			PlayAnimationTrack(3, 2.0f);
			if (IsAnimationFinished(3)) {
				animation = 0;
			}
			break;
		case 4:
			PlayAnimationTrack(4, 1.5f);
			if (IsAnimationFinished(4)) {
				animation = 0;
			}
			break;
		case 5:
			PlayAnimationTrack(5, 1.5f);
			if (IsAnimationFinished(5)) {
				animation = 0;
			}			
			break;
		case 6:
			PlayAnimationTrack(6, 1.5f);
			if (IsAnimationFinished(6)) {
				animation = 0;
			}
			break;
		case 2:
			PlayAnimationTrack(2);
			break;
		case 1:
			PlayAnimationTrack(1);
			break;
		case 0:
		default:
			PlayAnimationTrack(0);
			break;
		}
	}
	CGameObject::Animate(fTimeElapsed);
}

void OtherPlayer::OnPrepareRender()
{
	m_xmf4x4ToParent._11 = m_xmf3Right.x; m_xmf4x4ToParent._12 = m_xmf3Right.y; m_xmf4x4ToParent._13 = m_xmf3Right.z;
	m_xmf4x4ToParent._21 = m_xmf3Up.x; m_xmf4x4ToParent._22 = m_xmf3Up.y; m_xmf4x4ToParent._23 = m_xmf3Up.z;
	m_xmf4x4ToParent._31 = m_xmf3Look.x; m_xmf4x4ToParent._32 = m_xmf3Look.y; m_xmf4x4ToParent._33 = m_xmf3Look.z;
	//m_xmf4x4ToParent._41 = m_xmf3Position.x; m_xmf4x4ToParent._42 = m_xmf3Position.y; m_xmf4x4ToParent._43 = m_xmf3Position.z;

	m_xmf4x4ToParent = Matrix4x4::Multiply(XMMatrixScaling(m_xmf3Scale.x, m_xmf3Scale.y, m_xmf3Scale.z), m_xmf4x4ToParent);
}

void OtherPlayer::PlayAnimationTrack(int trackIndex, float speed)
{
	if (currentAnim == trackIndex) return;

	StartAnimationBlend(currentAnim, trackIndex, 0.3f);
	m_pSkinnedAnimationController->SetTrackSpeed(trackIndex, speed);

	currentAnim = trackIndex;
}

bool OtherPlayer::IsAnimationFinished(int trackIndex)
{
	float current = m_pSkinnedAnimationController->m_pAnimationTracks[trackIndex].m_fPosition;
	float length = m_pSkinnedAnimationController->m_pAnimationSets->m_pAnimationSets[trackIndex]->m_fLength;
	return current >= length;
}

void OtherPlayer::StartAnimationBlend(int fromTrack, int toTrack, float blendTime)
{
	if (!g_bAnimationBlendEnabled)
	{
		// Blending disabled: cut straight to the new track, no crossfade.
		m_animBlend.active = false;
		for (int i = 0; i < 7; ++i)
			m_pSkinnedAnimationController->SetTrackEnable(i, i == toTrack);
		m_pSkinnedAnimationController->SetTrackWeight(toTrack, 1.0f);
		m_pSkinnedAnimationController->SetTrackPosition(fromTrack, 0.0f);
		return;
	}

	m_animBlend.from = fromTrack;
	m_animBlend.to = toTrack;
	m_animBlend.duration = blendTime;
	m_animBlend.elapsed = 0.0f;
	m_animBlend.active = true;

	for (int i = 0; i < 7; ++i)
		m_pSkinnedAnimationController->SetTrackEnable(i, i == fromTrack || i == toTrack);

	m_pSkinnedAnimationController->SetTrackWeight(fromTrack, 1.0f);
	m_pSkinnedAnimationController->SetTrackWeight(toTrack, 0.0f);

}
