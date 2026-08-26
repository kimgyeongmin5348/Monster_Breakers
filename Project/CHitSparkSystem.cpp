#include "CHitSparkSystem.h"
#include "CFireballShader.h"
#include "Scene.h"
#include "d3dx12.h"
#include <random>

// -------------------------------------------------------
CHitSparkSystem::CHitSparkSystem(
    ID3D12Device* pd3dDevice,
    ID3D12GraphicsCommandList* pd3dCommandList,
    ID3D12RootSignature* pd3dRootSignature)
{
    CreateQuadMesh(pd3dDevice);
    CreateParticleBuffer(pd3dDevice);

    // Same pipeline as Fireball, only the pixel shader entry point (tint/falloff) differs.
    m_pShader = new CHitSparkShader();
    m_pShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dRootSignature);
    m_pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

    // Reuse the fire spritesheet; the shader tints it white/yellow instead of orange.
    m_pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
    m_pTexture->LoadTextureFromDDSFile(
        pd3dDevice, pd3dCommandList,
        L"Image/fire_spritesheet.dds", RESOURCE_TEXTURE2D, 0);

    CScene::CreateShaderResourceViews(pd3dDevice, m_pTexture, 0, 15);
}

// -------------------------------------------------------
void CHitSparkSystem::CreateQuadMesh(ID3D12Device* pd3dDevice)
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
void CHitSparkSystem::CreateParticleBuffer(ID3D12Device* pd3dDevice)
{
    UINT bufSize = sizeof(FireballParticleData) * MAX_PARTICLES;

    {
        CD3DX12_HEAP_PROPERTIES prop(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC   desc = CD3DX12_RESOURCE_DESC::Buffer(bufSize);
        pd3dDevice->CreateCommittedResource(
            &prop, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_pParticleUploadBuffer));
        m_pParticleUploadBuffer->Map(0, nullptr, (void**)&m_pMappedData);
    }

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
// Emit: spray `count` particles outward from position in random directions.
void CHitSparkSystem::Emit(XMFLOAT3 position, int count, float classTint)
{
    static std::mt19937 rng(777);
    std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
    std::uniform_real_distribution<float> speed(3.0f, 7.0f);
    std::uniform_real_distribution<float> life(0.18f, 0.32f);
    std::uniform_real_distribution<float> sz(0.25f, 0.5f);

    for (int b = 0; b < count; ++b)
    {
        for (int i = 0; i < MAX_PARTICLES; ++i)
        {
            int slot = (m_nNextSlot + i) % MAX_PARTICLES;
            if (!m_Particles[slot].active)
            {
                auto& p = m_Particles[slot];

                XMVECTOR vDir = XMVectorSet(dir(rng), dir(rng) * 0.6f + 0.4f, dir(rng), 0.0f);
                vDir = XMVector3Normalize(vDir);
                XMStoreFloat3(&p.velocity, vDir * XMVectorReplicate(speed(rng)));

                p.position = position;
                p.size = sz(rng);
                p.lifetime = 0.0f;
                p.maxLifetime = life(rng);
                p.uvOffset = 0.0f;
                p.active = 1;
                p.pad = classTint;

                m_nNextSlot = (slot + 1) % MAX_PARTICLES;
                m_bNeedUpload = true;
                break;
            }
        }
    }
}

// -------------------------------------------------------
void CHitSparkSystem::Animate(float fTimeElapsed)
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

        p.position.x += p.velocity.x * fTimeElapsed;
        p.position.y += p.velocity.y * fTimeElapsed;
        p.position.z += p.velocity.z * fTimeElapsed;

        // Strong drag: sparks snap outward then stop almost immediately.
        p.velocity.x *= (1.0f - 6.0f * fTimeElapsed);
        p.velocity.y -= 4.0f * fTimeElapsed;
        p.velocity.z *= (1.0f - 6.0f * fTimeElapsed);

        p.uvOffset = (p.lifetime * 8.0f / 16.0f);

        m_bNeedUpload = true;
    }
}

// -------------------------------------------------------
void CHitSparkSystem::UploadToGPU(ID3D12GraphicsCommandList* pd3dCommandList)
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
void CHitSparkSystem::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    UploadToGPU(pd3dCommandList);

    m_pShader->Render(pd3dCommandList, pCamera);

    if (m_pTexture)
        m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);

    pd3dCommandList->SetGraphicsRootShaderResourceView(19, m_pParticleDefaultBuffer->GetGPUVirtualAddress());

    pd3dCommandList->IASetVertexBuffers(0, 1, &m_QuadVBView);
    pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dCommandList->DrawInstanced(6, MAX_PARTICLES, 0, 0);
}

// -------------------------------------------------------
int CHitSparkSystem::GetActiveCount() const
{
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES; ++i)
        if (m_Particles[i].active) ++count;
    return count;
}

// -------------------------------------------------------
CHitSparkSystem::~CHitSparkSystem()
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
