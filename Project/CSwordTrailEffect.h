#pragma once
#include "stdafx.h"

// A single ring vertex: world-space position, a UV used purely for procedural
// shading (x = position along the arc, y = across the ribbon's width), and a
// CPU-driven alpha used for both the comet-trail gradient and the lifetime fade.
struct SwordTrailVertex
{
    XMFLOAT3 position;
    XMFLOAT2 uv;
    float     alpha;
};

class CCamera;

// Procedural ribbon mesh tracing an arc around a pivot - a glowing motion trail
// for a sword sweep, not a billboard particle system. Modeled after
// CGroundCrackEffect's "build vertices on the CPU every frame" approach.
class CSwordTrailEffect
{
public:
    CSwordTrailEffect() = default;
    ~CSwordTrailEffect();

    void Create(ID3D12Device* pd3dDevice,
        ID3D12GraphicsCommandList* pd3dCommandList,
        ID3D12RootSignature* pd3dGraphicsRootSignature);

    // origin: sweep pivot (roughly weapon/chest height).
    // forward/right: horizontal basis spanning the swing plane.
    // radius: blade reach. arcDegrees: total sweep angle (can exceed 360 for a full spin).
    // delaySeconds: wait this long before the trail actually appears (e.g. to line
    // up with a swing animation's windup instead of firing on the keypress).
    void Trigger(const XMFLOAT3& origin, const XMFLOAT3& forward, const XMFLOAT3& right,
        float radius, float arcDegrees, float delaySeconds = 0.0f);

    void Update(float fTimeElapsed);
    void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

    bool IsActive() const { return m_bActive; }

private:
    void UpdateVertices(float t);

    bool  m_bActive = false;
    float m_fTimer = 0.0f;
    float m_fDuration = 0.45f;

    XMFLOAT3 m_xmf3Origin = {};
    XMFLOAT3 m_xmf3Forward = { 0.0f, 0.0f, 1.0f };
    XMFLOAT3 m_xmf3Right = { 1.0f, 0.0f, 0.0f };
    float    m_fRadius = 2.2f;
    float    m_fArcRadians = XM_2PI;

    // A trigger with a delay just parks its parameters here until the delay elapses.
    bool     m_bPending = false;
    float    m_fPendingDelay = 0.0f;
    XMFLOAT3 m_xmf3PendingOrigin = {};
    XMFLOAT3 m_xmf3PendingForward = {};
    XMFLOAT3 m_xmf3PendingRight = {};
    float    m_fPendingRadius = 0.0f;
    float    m_fPendingArcDegrees = 0.0f;

    static constexpr int   SEGMENTS = 32;
    static constexpr int   VERTS_PER_RING = 2;                             // inner/outer edge
    static constexpr int   MAX_VERTS = (SEGMENTS + 1) * VERTS_PER_RING;
    static constexpr int   MAX_INDICES = SEGMENTS * 6;

    ID3D12Resource* m_pVertexUploadBuffer = nullptr;
    SwordTrailVertex* m_pMappedVerts = nullptr;
    D3D12_VERTEX_BUFFER_VIEW m_VBView = {};

    ID3D12Resource* m_pIndexBuffer = nullptr;
    D3D12_INDEX_BUFFER_VIEW  m_IBView = {};

    ID3D12PipelineState* m_pPipelineState = nullptr;
};
