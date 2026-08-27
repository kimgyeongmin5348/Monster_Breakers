#pragma once
#include "stdafx.h"
#include "Shader.h"
#include "Mesh.h"
#include "CFireballSystem.h"   // reuse FireballParticleData

// Quick omnidirectional spark burst used for weapon/skill impacts and the
// thief's spin-slash visual. Same GPU-instanced-quad pipeline as
// CFireballSystem/CGreenSpiritSystem, just a different emit pattern and tint.
class CHitSparkSystem
{
public:
    static const int MAX_PARTICLES = 256;

    CHitSparkSystem(
        ID3D12Device* pd3dDevice,
        ID3D12GraphicsCommandList* pd3dCommandList,
        ID3D12RootSignature* pd3dRootSignature);

    ~CHitSparkSystem();

    // Class tint IDs consumed by PSHitSpark (Shaders.hlsl) - keep in sync.
    enum ClassTint
    {
        TINT_DEFAULT = 0,
        TINT_KNIGHT = 1,  // blue
        TINT_THIEF = 2,   // gray
        TINT_WIZARD = 3,  // purple
    };

    // Spawns a short-lived radial burst of sparks centered on position.
    void Emit(XMFLOAT3 position, int count = 10, float classTint = TINT_DEFAULT);
    void Animate(float fTimeElapsed);
    void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
    void ReleaseUploadBuffers() {}

    int GetActiveCount() const;

private:
    void CreateQuadMesh(ID3D12Device* pd3dDevice);
    void CreateParticleBuffer(ID3D12Device* pd3dDevice);
    void UploadToGPU(ID3D12GraphicsCommandList* pd3dCommandList);

private:
    FireballParticleData m_Particles[MAX_PARTICLES] = {};
    int  m_nNextSlot = 0;
    bool m_bNeedUpload = false;

    ID3D12Resource* m_pParticleUploadBuffer = nullptr;
    FireballParticleData* m_pMappedData = nullptr;
    ID3D12Resource* m_pParticleDefaultBuffer = nullptr;

    ID3D12Resource* m_pQuadVB = nullptr;
    D3D12_VERTEX_BUFFER_VIEW m_QuadVBView = {};

    CShader* m_pShader = nullptr;
    CTexture* m_pTexture = nullptr;
};
