#include "stdafx.h"
#include "Camera.h"
#include "Player.h"

CCamera::CCamera()
{
	m_xmf4x4View = Matrix4x4::Identity();
	m_xmf4x4Projection = Matrix4x4::Identity();
	m_d3dViewport = { 0, 0, FRAME_BUFFER_WIDTH , FRAME_BUFFER_HEIGHT, 0.0f, 1.0f };
	m_d3dScissorRect = { 0, 0, FRAME_BUFFER_WIDTH , FRAME_BUFFER_HEIGHT };
	m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);
	m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_fPitch = 0.0f;
	m_fRoll = 0.0f;
	m_fYaw = 0.0f;
	m_xmf3Offset = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_fTimeLag = 0.0f;
	m_xmf3LookAtWorld = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_nMode = 0x00;
	m_pPlayer = NULL;
}

CCamera::CCamera(CCamera* pCamera)
{
	if (pCamera)
	{
		*this = *pCamera;
	}
	else
	{
		m_xmf4x4View = Matrix4x4::Identity();
		m_xmf4x4Projection = Matrix4x4::Identity();
		m_d3dViewport = { 0, 0, FRAME_BUFFER_WIDTH , FRAME_BUFFER_HEIGHT, 0.0f, 1.0f };
		m_d3dScissorRect = { 0, 0, FRAME_BUFFER_WIDTH , FRAME_BUFFER_HEIGHT };
		m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
		m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);
		m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
		m_fPitch = 0.0f;
		m_fRoll = 0.0f;
		m_fYaw = 0.0f;
		m_xmf3Offset = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_fTimeLag = 0.0f;
		m_xmf3LookAtWorld = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_nMode = 0x00;
		m_pPlayer = NULL;
	}
}

CCamera::~CCamera()
{}

void CCamera::SetViewport(int xTopLeft, int yTopLeft, int nWidth, int nHeight, float fMinZ, float fMaxZ)
{
	m_d3dViewport.TopLeftX = float(xTopLeft);
	m_d3dViewport.TopLeftY = float(yTopLeft);
	m_d3dViewport.Width = float(nWidth);
	m_d3dViewport.Height = float(nHeight);
	m_d3dViewport.MinDepth = fMinZ;
	m_d3dViewport.MaxDepth = fMaxZ;
}

void CCamera::SetScissorRect(LONG xLeft, LONG yTop, LONG xRight, LONG yBottom)
{
	m_d3dScissorRect.left = xLeft;
	m_d3dScissorRect.top = yTop;
	m_d3dScissorRect.right = xRight;
	m_d3dScissorRect.bottom = yBottom;
}

void CCamera::GenerateProjectionMatrix(float fNearPlaneDistance, float fFarPlaneDistance, float fAspectRatio, float fFOVAngle)
{
	m_xmf4x4Projection = Matrix4x4::PerspectiveFovLH(XMConvertToRadians(fFOVAngle), fAspectRatio, fNearPlaneDistance, fFarPlaneDistance);
	//	XMMATRIX xmmtxProjection = XMMatrixPerspectiveFovLH(XMConvertToRadians(fFOVAngle), fAspectRatio, fNearPlaneDistance, fFarPlaneDistance);
	//	XMStoreFloat4x4(&m_xmf4x4Projection, xmmtxProjection);
}

void CCamera::GenerateViewMatrix(XMFLOAT3 xmf3Position, XMFLOAT3 xmf3LookAt, XMFLOAT3 xmf3Up)
{
	m_xmf3Position = xmf3Position;
	m_xmf3LookAtWorld = xmf3LookAt;
	m_xmf3Up = xmf3Up;

	GenerateViewMatrix();
}

void CCamera::GenerateViewMatrix()
{
	m_xmf4x4View = Matrix4x4::LookAtLH(m_xmf3Position, m_xmf3LookAtWorld, m_xmf3Up);
}

void CCamera::RegenerateViewMatrix()
{
	m_xmf3Look = Vector3::Normalize(m_xmf3Look);
	m_xmf3Right = Vector3::CrossProduct(m_xmf3Up, m_xmf3Look, true);
	m_xmf3Up = Vector3::CrossProduct(m_xmf3Look, m_xmf3Right, true);

	m_xmf4x4View._11 = m_xmf3Right.x; m_xmf4x4View._12 = m_xmf3Up.x; m_xmf4x4View._13 = m_xmf3Look.x;
	m_xmf4x4View._21 = m_xmf3Right.y; m_xmf4x4View._22 = m_xmf3Up.y; m_xmf4x4View._23 = m_xmf3Look.y;
	m_xmf4x4View._31 = m_xmf3Right.z; m_xmf4x4View._32 = m_xmf3Up.z; m_xmf4x4View._33 = m_xmf3Look.z;
	m_xmf4x4View._41 = -Vector3::DotProduct(m_xmf3Position, m_xmf3Right);
	m_xmf4x4View._42 = -Vector3::DotProduct(m_xmf3Position, m_xmf3Up);
	m_xmf4x4View._43 = -Vector3::DotProduct(m_xmf3Position, m_xmf3Look);
}

void CCamera::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(VS_CB_CAMERA_INFO) + 255) & ~255); //256�� ���
	m_pd3dcbCamera = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbCamera->Map(0, NULL, (void**)&m_pcbMappedCamera);
}

void CCamera::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	XMFLOAT4X4 xmf4x4View;
	XMStoreFloat4x4(&xmf4x4View, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4View)));
	::memcpy(&m_pcbMappedCamera->m_xmf4x4View, &xmf4x4View, sizeof(XMFLOAT4X4));

	XMFLOAT4X4 xmf4x4Projection;
	XMStoreFloat4x4(&xmf4x4Projection, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4Projection)));
	::memcpy(&m_pcbMappedCamera->m_xmf4x4Projection, &xmf4x4Projection, sizeof(XMFLOAT4X4));

	::memcpy(&m_pcbMappedCamera->m_xmf3Position, &m_xmf3Position, sizeof(XMFLOAT3));

	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbCamera->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(0, d3dGpuVirtualAddress);
}

void CCamera::ReleaseShaderVariables()
{
	if (m_pd3dcbCamera)
	{
		m_pd3dcbCamera->Unmap(0, NULL);
		m_pd3dcbCamera->Release();
	}
}

void CCamera::SetViewportsAndScissorRects(ID3D12GraphicsCommandList* pd3dCommandList)
{
	pd3dCommandList->RSSetViewports(1, &m_d3dViewport);
	pd3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSpaceShipCamera

CSpaceShipCamera::CSpaceShipCamera(CCamera* pCamera) : CCamera(pCamera)
{
	m_nMode = SPACESHIP_CAMERA;
}

void CSpaceShipCamera::Rotate(float x, float y, float z)
{
	if (m_pPlayer && (x != 0.0f))
	{
		XMFLOAT3 xmf3Right = m_pPlayer->GetRightVector();
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Right), XMConvertToRadians(x));
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
		m_xmf3Position = Vector3::Subtract(m_xmf3Position, m_pPlayer->GetPosition());
		m_xmf3Position = Vector3::TransformCoord(m_xmf3Position, xmmtxRotate);
		m_xmf3Position = Vector3::Add(m_xmf3Position, m_pPlayer->GetPosition());
	}
	if (m_pPlayer && (y != 0.0f))
	{
		XMFLOAT3 xmf3Up = m_pPlayer->GetUpVector();
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Up), XMConvertToRadians(y));
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
		m_xmf3Position = Vector3::Subtract(m_xmf3Position, m_pPlayer->GetPosition());
		m_xmf3Position = Vector3::TransformCoord(m_xmf3Position, xmmtxRotate);
		m_xmf3Position = Vector3::Add(m_xmf3Position, m_pPlayer->GetPosition());
	}
	if (m_pPlayer && (z != 0.0f))
	{
		XMFLOAT3 xmf3Look = m_pPlayer->GetLookVector();
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Look), XMConvertToRadians(z));
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
		m_xmf3Position = Vector3::Subtract(m_xmf3Position, m_pPlayer->GetPosition());
		m_xmf3Position = Vector3::TransformCoord(m_xmf3Position, xmmtxRotate);
		m_xmf3Position = Vector3::Add(m_xmf3Position, m_pPlayer->GetPosition());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFirstPersonCamera

CFirstPersonCamera::CFirstPersonCamera(CCamera* pCamera) : CCamera(pCamera)
{
	m_nMode = FIRST_PERSON_CAMERA;
	if (pCamera)
	{
		if (pCamera->GetMode() == SPACESHIP_CAMERA)
		{
			m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
			m_xmf3Right.y = 0.0f;
			m_xmf3Look.y = 0.0f;
			m_xmf3Right = Vector3::Normalize(m_xmf3Right);
			m_xmf3Look = Vector3::Normalize(m_xmf3Look);
		}
	}
}

void CFirstPersonCamera::Rotate(float x, float y, float z)
{
	if (x != 0.0f)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Right), XMConvertToRadians(x));
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
	}
	if (m_pPlayer && (y != 0.0f))
	{
		XMFLOAT3 xmf3Up = m_pPlayer->GetUpVector();
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Up), XMConvertToRadians(y));
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
	}
	if (m_pPlayer && (z != 0.0f))
	{
		XMFLOAT3 xmf3Look = m_pPlayer->GetLookVector();
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Look), XMConvertToRadians(z));
		//m_xmf3Position = Vector3::Subtract(m_xmf3Position, m_pPlayer->GetPosition());
		//m_xmf3Position = Vector3::TransformCoord(m_xmf3Position, xmmtxRotate);
		//m_xmf3Position = Vector3::Add(m_xmf3Position, m_pPlayer->GetPosition());
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CThirdPersonCamera

CThirdPersonCamera::CThirdPersonCamera(CCamera* pCamera) : CCamera(pCamera)
{
	m_nMode = THIRD_PERSON_CAMERA;
	if (pCamera)
	{
		if (pCamera->GetMode() == SPACESHIP_CAMERA)
		{
			m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
			m_xmf3Right.y = 0.0f;
			m_xmf3Look.y = 0.0f;
			m_xmf3Right = Vector3::Normalize(m_xmf3Right);
			m_xmf3Look = Vector3::Normalize(m_xmf3Look);
		}
	}
}

void CThirdPersonCamera::Update(XMFLOAT3& xmf3LookAt, float fTimeElapsed)
{
	if (m_pPlayer)
	{
		// 카메라는 더 이상 플레이어의 Right/Up/Look(몸통 방향)을 기준으로 궤도를 돌지 않는다.
		// 대신 마우스로만 조작되는 카메라 자체의 궤도각(m_fOrbitYaw/m_fOrbitPitch)을 사용한다.
		// -> WASD로 몸통이 돌아도 시점은 그대로 유지되고, 시점은 오직 마우스로만 회전한다.
		XMMATRIX xmmtxYaw = XMMatrixRotationY(XMConvertToRadians(m_fOrbitYaw));
		XMMATRIX xmmtxPitch = XMMatrixRotationX(XMConvertToRadians(m_fOrbitPitch));
		XMMATRIX xmmtxRotate = XMMatrixMultiply(xmmtxPitch, xmmtxYaw);

		XMFLOAT3 mxf3PlayerPosition = m_pPlayer->GetPosition();
		XMFLOAT3 xmf3Offset = Vector3::TransformCoord(m_xmf3Offset, xmmtxRotate);
		XMFLOAT3 xmf3Position = Vector3::Add(mxf3PlayerPosition, xmf3Offset);
		XMFLOAT3 xmf3Direction = Vector3::Subtract(xmf3Position, m_xmf3Position);
		float fLength = Vector3::Length(xmf3Direction);
		xmf3Direction = Vector3::Normalize(xmf3Direction);
		float fTimeLagScale = (m_fTimeLag) ? fTimeElapsed * (1.0f / m_fTimeLag) : 1.0f;
		float fDistance = fLength * fTimeLagScale;
		if (fDistance > fLength) fDistance = fLength;
		if (fLength < 0.01f) fDistance = fLength;
		if (fDistance > 0)
		{
			m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Direction, fDistance);
		}

		UpdateShake(fTimeElapsed);

		XMFLOAT3 xmf3AdjustedLookAt = xmf3LookAt;
		xmf3AdjustedLookAt.y += 1.5f;
		SetLookAt(xmf3AdjustedLookAt);
	
		//GenerateViewMatrix();



		//UpdateShake(fTimeElapsed);

		//XMFLOAT3 xmf3AdjustedLookAt = xmf3LookAt;
		//xmf3AdjustedLookAt.y += 1.5f;

		//// 흔들림 회전을 look-at 지점에도 살짝 반영하고 싶다면
		//// (간단히는 lookAt 자체에 회전 오프셋을 더하는 방식)
		////xmf3AdjustedLookAt.x += m_xmf3ShakeRotation.y * someScale; // yaw 흔들림 → 좌우
		////xmf3AdjustedLookAt.y += m_xmf3ShakeRotation.x * someScale; // pitch 흔들림 → 상하

		//SetLookAt(xmf3AdjustedLookAt);

		//// 최종 렌더용 위치 = 순수 위치 + 흔들림 오프셋
		//m_xmf3RenderPosition = Vector3::Add(m_xmf3Position, m_xmf3ShakeOffset);

		//GenerateViewMatrix(); // 이 함수가 m_xmf3Position 대신 m_xmf3RenderPosition을 쓰도록 수정 필요
	}
}

void CThirdPersonCamera::SetLookAt(XMFLOAT3& xmf3LookAt)
{
	XMFLOAT4X4 mtxLookAt = Matrix4x4::LookAtLH(m_xmf3Position, xmf3LookAt, m_pPlayer->GetUpVector());
	m_xmf3Right = XMFLOAT3(mtxLookAt._11, mtxLookAt._21, mtxLookAt._31);
	m_xmf3Up = XMFLOAT3(mtxLookAt._12, mtxLookAt._22, mtxLookAt._32);
	m_xmf3Look = XMFLOAT3(mtxLookAt._13, mtxLookAt._23, mtxLookAt._33);

	//m_xmf3Look = Vector3::Normalize(Vector3::Subtract(xmf3LookAt, m_xmf3Position));
	//m_xmf3Right = Vector3::Normalize(Vector3::CrossProduct(XMFLOAT3(0.0f, 1.0f, 0.0f), m_xmf3Look));
	//m_xmf3Up = Vector3::Normalize(Vector3::CrossProduct(m_xmf3Look, m_xmf3Right));
}

void CThirdPersonCamera::Rotate(float fPitch, float fYaw, float fRoll)
{
	if (!m_pPlayer) return;

	if (fPitch != 0.0f)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Right), XMConvertToRadians(fPitch));
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
	}

	if (fYaw != 0.0f)
	{
		XMFLOAT3 xmf3Up = m_pPlayer->GetUpVector();
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&xmf3Up), XMConvertToRadians(fYaw));
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
	}

	if (fRoll != 0.0f)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Look), XMConvertToRadians(fRoll));
		m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
	}
}

void CThirdPersonCamera::AddOrbitRotation(float deltaYaw, float deltaPitch)
{
	m_fOrbitYaw += deltaYaw;
	if (m_fOrbitYaw > 360.0f) m_fOrbitYaw -= 360.0f;
	if (m_fOrbitYaw < 0.0f)   m_fOrbitYaw += 360.0f;

	m_fOrbitPitch += deltaPitch;
	if (m_fOrbitPitch > ORBIT_PITCH_MAX) m_fOrbitPitch = ORBIT_PITCH_MAX;
	if (m_fOrbitPitch < ORBIT_PITCH_MIN) m_fOrbitPitch = ORBIT_PITCH_MIN;
}

void CThirdPersonCamera::StartShake(float duration, float magnitude, float frequency)
{
	m_fShakeTime = 0.0f;
	m_fShakeDuration = duration;
	m_fShakeMagnitude = magnitude;
	m_fShakeFrequency = frequency;

	m_xmf3ShakeOffset = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3ShakeRotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void CThirdPersonCamera::UpdateShake(float fTimeElapsed)
{
	if (m_fShakeTime >= m_fShakeDuration)
	{
		m_xmf3ShakeOffset = XMFLOAT3(0, 0, 0);
		m_xmf3ShakeRotation = XMFLOAT3(0, 0, 0);
		return;
	}

	m_fShakeTime += fTimeElapsed;

	float t = m_fShakeTime / m_fShakeDuration;
	t = std::min(t, 1.0f);

	float falloff = 1.0f - t;
	falloff *= falloff;

	float time = m_fShakeTime * m_fShakeFrequency;

	float x = sinf(time);
	float y = sinf(time * 1.37f + 1.7f);
	float z = sinf(time * 0.83f + 3.2f);

	m_xmf3ShakeOffset.x = x * m_fShakeMagnitude * falloff;
	m_xmf3ShakeOffset.y = y * m_fShakeMagnitude * falloff;
	m_xmf3ShakeOffset.z = z * m_fShakeMagnitude * falloff;

	m_xmf3ShakeRotation.x = y * m_fShakeMagnitude * 2.0f * falloff;
	m_xmf3ShakeRotation.y = z * m_fShakeMagnitude * 1.5f * falloff;
	m_xmf3ShakeRotation.z = x * m_fShakeMagnitude * 2.5f * falloff;
}

void CThirdPersonCamera::RegenerateViewMatrix()
{
	XMVECTOR right = XMLoadFloat3(&m_xmf3Right);
	XMVECTOR up = XMLoadFloat3(&m_xmf3Up);
	XMVECTOR look = XMLoadFloat3(&m_xmf3Look);

	XMVECTOR position = XMLoadFloat3(&m_xmf3Position);

	// =====================================================
	// Camera Shake Rotation
	// =====================================================

	XMMATRIX shakeRotation =
		XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(m_xmf3ShakeRotation.x),
			XMConvertToRadians(m_xmf3ShakeRotation.y),
			XMConvertToRadians(m_xmf3ShakeRotation.z)
		);

	right = XMVector3TransformNormal(right,shakeRotation);

	up = XMVector3TransformNormal(up,shakeRotation);

	look = XMVector3TransformNormal(look,shakeRotation);

	// =====================================================
	// Camera Shake Position
	// =====================================================

	position +=	right * m_xmf3ShakeOffset.x;

	position +=	up * m_xmf3ShakeOffset.y;

	position +=	look * m_xmf3ShakeOffset.z;

	// =====================================================
	// Normalize
	// =====================================================

	right = XMVector3Normalize(right);
	up = XMVector3Normalize(up);
	look = XMVector3Normalize(look);

	// =====================================================
	// 기존 RegenerateViewMatrix()와 동일한 방식
	// =====================================================

	m_xmf4x4View._11 = XMVectorGetX(right);
	m_xmf4x4View._12 = XMVectorGetX(up);
	m_xmf4x4View._13 = XMVectorGetX(look);

	m_xmf4x4View._21 = XMVectorGetY(right);
	m_xmf4x4View._22 = XMVectorGetY(up);
	m_xmf4x4View._23 = XMVectorGetY(look);

	m_xmf4x4View._31 = XMVectorGetZ(right);
	m_xmf4x4View._32 = XMVectorGetZ(up);
	m_xmf4x4View._33 = XMVectorGetZ(look);

	m_xmf4x4View._41 = -XMVectorGetX(XMVector3Dot(position, right));
	m_xmf4x4View._42 = -XMVectorGetX(XMVector3Dot(position, up));
	m_xmf4x4View._43 = -XMVectorGetX(XMVector3Dot(position, look));

	m_xmf4x4View._44 = 1.0f;
}