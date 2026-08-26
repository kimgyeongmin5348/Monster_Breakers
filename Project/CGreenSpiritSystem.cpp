#include "CGreenSpiritSystem.h"
#include "CFireballShader.h"
#include "Scene.h"
#include "d3dx12.h"
#include <random>

// -------------------------------------------------------
CGreenSpiritSystem::CGreenSpiritSystem(
    ID3D12Device* pd3dDevice,
    ID3D12GraphicsCommandList* pd3dCommandList,
    ID3D12RootSignature* pd3dRootSignature)
{
    CreateQuadMesh(pd3dDevice);
    CreateParticleBuffer(pd3dDevice);

    // Fireball과 동일한 파이프라인 구조 사용, 셰이더 진입점만 다름
    m_pShader = new CGreenSpiritShader();
    m_pShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dRootSignature);
    m_pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

    // 불꽃 스프라이트시트를 그대로 재활용
    // (PSGreenSpirit에서 초록 틴트를 적용하므로 별도 텍스처 불필요)
    m_pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
    m_pTexture->LoadTextureFromDDSFile(
        pd3dDevice, pd3dCommandList,
        L"Image/fire_spritesheet.dds", RESOURCE_TEXTURE2D, 0);

    CScene::CreateShaderResourceViews(pd3dDevice, m_pTexture, 0, 15);
}

// -------------------------------------------------------
void CGreenSpiritSystem::CreateQuadMesh(ID3D12Device* pd3dDevice)
{
    CTexturedVertex quad[6] = {
        { XMFLOAT3(-0.5f,  0.5f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(0.5f,  0.5f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-0.5f,  0.5f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
    };

    UINT size = sizeof(quad);
    UINT stride = sizeof(CTexturedVertex);

    CD3DX12_HEAP_PROPERTIES uploadProp(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC   bufDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
    pd3dDevice->CreateCommittedResource(
        &uploadProp, D3D12_HEAP_FLAG_NONE, &bufDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_pQuadVB));

    void* pData = nullptr;
    m_pQuadVB->Map(0, nullptr, &pData);
    memcpy(pData, quad, size);
    m_pQuadVB->Unmap(0, nullptr);

    m_QuadVBView.BufferLocation = m_pQuadVB->GetGPUVirtualAddress();
    m_QuadVBView.StrideInBytes = stride;
    m_QuadVBView.SizeInBytes = size;
}

// -------------------------------------------------------
void CGreenSpiritSystem::CreateParticleBuffer(ID3D12Device* pd3dDevice)
{
    UINT bufSize = sizeof(FireballParticleData) * MAX_PARTICLES;

    // Upload 힙 (CPU → GPU 복사용, 영구 매핑)
    {
        CD3DX12_HEAP_PROPERTIES prop(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC   desc = CD3DX12_RESOURCE_DESC::Buffer(bufSize);
        pd3dDevice->CreateCommittedResource(
            &prop, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_pParticleUploadBuffer));
        m_pParticleUploadBuffer->Map(0, nullptr, (void**)&m_pMappedData);
    }

    // Default 힙 (GPU SRV t4 로 읽힘)
    {
        CD3DX12_HEAP_PROPERTIES prop(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC   desc = CD3DX12_RESOURCE_DESC::Buffer(
            bufSize, D3D12_RESOURCE_FLAG_NONE);
        pd3dDevice->CreateCommittedResource(
            &prop, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&m_pParticleDefaultBuffer));
    }
}

// -------------------------------------------------------
// Emit: 플레이어 발 위치(바닥)에서 위쪽으로 퍼져 오르는 파티클 방출
void CGreenSpiritSystem::Emit(XMFLOAT3 position)
{
    static std::mt19937 rng(99999);
    std::uniform_real_distribution<float> spread(-1.2f, 1.2f);
    std::uniform_real_distribution<float> upward(3.0f, 7.0f);  // 강한 상승력
    std::uniform_real_distribution<float> life(1.0f, 1.8f);
    std::uniform_real_distribution<float> sz(0.3f, 0.8f);

    // Q를 누를 때 한꺼번에 여러 파티클 방출 (폭발적 등장감)
    const int BURST = 12;
    for (int b = 0; b < BURST; ++b)
    {
        for (int i = 0; i < MAX_PARTICLES; ++i)
        {
            int slot = (m_nNextSlot + i) % MAX_PARTICLES;
            if (!m_Particles[slot].active)
            {
                auto& p = m_Particles[slot];

                // 발 아래 → 위로 솟구치도록 Y 속도를 크게 설정
                p.velocity = { spread(rng), upward(rng), spread(rng) };
                p.position = position;               // 바닥 위치
                p.size = sz(rng);
                p.lifetime = 0.0f;
                p.maxLifetime = life(rng);
                p.uvOffset = 0.0f;
                p.active = 1;
                p.pad = 0.0f;

                m_nNextSlot = (slot + 1) % MAX_PARTICLES;
                m_bNeedUpload = true;
                break;
            }
        }
    }
}

// -------------------------------------------------------
void CGreenSpiritSystem::Animate(float fTimeElapsed)
{
    for (auto& p : m_Particles)
    {
        if (!p.active) continue;

        p.lifetime += fTimeElapsed;
        if (p.lifetime >= p.maxLifetime)
        {
            p.active = 0;
            m_bNeedUpload = true;
            continue;
        }

        // 위치 이동
        p.position.x += p.velocity.x * fTimeElapsed;
        p.position.y += p.velocity.y * fTimeElapsed;
        p.position.z += p.velocity.z * fTimeElapsed;

        float lifeRatio = p.lifetime / p.maxLifetime;

        // 상승하다가 수명 후반엔 서서히 감속 (중력은 약하게)
        p.velocity.y -= 1.5f * fTimeElapsed;

        // 수평 속도 감쇠 (유령처럼 흐릿하게 퍼짐)
        p.velocity.x *= (1.0f - 0.8f * fTimeElapsed);
        p.velocity.z *= (1.0f - 0.8f * fTimeElapsed);

        // 수명에 따른 좌우 너울거림 (영혼 느낌)
        float wave = sinf(p.lifetime * 5.0f + p.position.x * 2.0f) * 0.5f;
        p.velocity.x += wave * fTimeElapsed;

        // 스프라이트 시트 스크롤 (Fireball과 동일 속도)
        p.uvOffset = (p.lifetime * 8.0f / 16.0f);

        m_bNeedUpload = true;
    }
}

// -------------------------------------------------------
void CGreenSpiritSystem::UploadToGPU(ID3D12GraphicsCommandList* pd3dCommandList)
{
    if (!m_bNeedUpload) return;
    m_bNeedUpload = false;

    memcpy(m_pMappedData, m_Particles, sizeof(m_Particles));

    auto toCopy = CD3DX12_RESOURCE_BARRIER::Transition(
        m_pParticleDefaultBuffer,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_COPY_DEST);
    pd3dCommandList->ResourceBarrier(1, &toCopy);

    pd3dCommandList->CopyBufferRegion(
        m_pParticleDefaultBuffer, 0,
        m_pParticleUploadBuffer, 0,
        sizeof(m_Particles));

    auto toRead = CD3DX12_RESOURCE_BARRIER::Transition(
        m_pParticleDefaultBuffer,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ);
    pd3dCommandList->ResourceBarrier(1, &toRead);
}

// -------------------------------------------------------
void CGreenSpiritSystem::Render(
    ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    UploadToGPU(pd3dCommandList);

    m_pShader->Render(pd3dCommandList, pCamera);

    if (m_pTexture)
        m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);

    // Fireball과 동일한 루트 파라미터 19번 (t4) 에 파티클 버퍼 바인딩
    pd3dCommandList->SetGraphicsRootShaderResourceView(
        19, m_pParticleDefaultBuffer->GetGPUVirtualAddress());

    pd3dCommandList->IASetVertexBuffers(0, 1, &m_QuadVBView);
    pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dCommandList->DrawInstanced(6, MAX_PARTICLES, 0, 0);
    ++g_nDrawCallCount;
}

// -------------------------------------------------------
int CGreenSpiritSystem::GetActiveCount() const
{
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES; ++i)
        if (m_Particles[i].active) ++count;
    return count;
}

// -------------------------------------------------------
CGreenSpiritSystem::~CGreenSpiritSystem()
{
    if (m_pParticleUploadBuffer)
    {
        m_pParticleUploadBuffer->Unmap(0, nullptr);
        m_pParticleUploadBuffer->Release();
    }
    if (m_pParticleDefaultBuffer) m_pParticleDefaultBuffer->Release();
    if (m_pQuadVB)                m_pQuadVB->Release();
    if (m_pShader) { delete m_pShader;  m_pShader = nullptr; }
    if (m_pTexture) { delete m_pTexture; m_pTexture = nullptr; }
}