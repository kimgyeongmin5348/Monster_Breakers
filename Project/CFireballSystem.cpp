#include "CFireballSystem.h"
#include "CFireballShader.h"
#include "Scene.h"
#include "d3dx12.h"        // 추가: CD3DX12_* 헬퍼 타입 정의 포함
#include <random>

// -------------------------------------------------------
CFireballSystem::CFireballSystem(
    ID3D12Device* pd3dDevice,
    ID3D12GraphicsCommandList* pd3dCommandList,
    ID3D12RootSignature* pd3dRootSignature)
{
    CreateQuadMesh(pd3dDevice);
    CreateParticleBuffer(pd3dDevice);

    // ── 쉐이더 ────────────────────────────────────────
    m_pShader = new CFireballShader();
    m_pShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dRootSignature);
    m_pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

    m_pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
    m_pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/fire_spritesheet.dds", RESOURCE_TEXTURE2D, 0);

    CScene::CreateShaderResourceViews(pd3dDevice, m_pTexture, 0, 15);
}

// -------------------------------------------------------
void CFireballSystem::CreateQuadMesh(ID3D12Device* pd3dDevice)
{
    // 로컬 공간 기준 단위 쿼드 (-0.5 ~ +0.5)
    // VS에서 카메라 Right/Up 벡터로 빌보드 변환함
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
void CFireballSystem::CreateParticleBuffer(ID3D12Device* pd3dDevice)
{
    UINT bufSize = sizeof(FireballParticleData) * MAX_PARTICLES;

    // 업로드 힙: CPU가 매 프레임 memcpy, 영구 매핑 유지
    {
        CD3DX12_HEAP_PROPERTIES prop(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC   desc = CD3DX12_RESOURCE_DESC::Buffer(bufSize);
        pd3dDevice->CreateCommittedResource(
            &prop, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_pParticleUploadBuffer));

        m_pParticleUploadBuffer->Map(0, nullptr, (void**)&m_pMappedData);
    }

    // Default 힙: GPU SRV(t6)로 읽음
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
void CFireballSystem::Emit(XMFLOAT3 position, XMFLOAT3 direction, float speed)
{
    static std::mt19937 rng(12345);
    std::uniform_real_distribution<float> spread(-1.5f, 1.5f);
    std::uniform_real_distribution<float> life(1.2f, 2.0f);
    std::uniform_real_distribution<float> sz(0.5f, 1.1f);

    // 풀 순환 탐색: 비활성 슬롯 발견하면 활성화
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        int slot = (m_nNextSlot + i) % MAX_PARTICLES;
        if (!m_Particles[slot].active)
        {
            auto& p = m_Particles[slot];

            XMVECTOR vDir = XMVector3Normalize(XMLoadFloat3(&direction));
            XMStoreFloat3(&p.velocity, vDir * XMVectorReplicate(speed));
            p.velocity.x += spread(rng);
            p.velocity.y += spread(rng) * 0.4f;
            p.velocity.z += spread(rng);

            p.position = position;
            p.size = sz(rng);
            p.lifetime = 0.0f;
            p.maxLifetime = life(rng);
            p.uvOffset = 0.0f;
            p.active = 1;

            m_nNextSlot = (slot + 1) % MAX_PARTICLES;
            m_bNeedUpload = true;
            return;
        }
    }
    // 슬롯이 꽉 찼으면 가장 오래된 슬롯 강제 재사용
    auto& p = m_Particles[m_nNextSlot];
    XMVECTOR vDir = XMVector3Normalize(XMLoadFloat3(&direction));
    XMStoreFloat3(&p.velocity, vDir * XMVectorReplicate(speed));
    p.position = position;
    p.size = sz(rng);
    p.lifetime = 0.0f;
    p.maxLifetime = life(rng);
    p.uvOffset = 0.0f;
    p.active = 1;
    m_nNextSlot = (m_nNextSlot + 1) % MAX_PARTICLES;
    m_bNeedUpload = true;
}

// -------------------------------------------------------
void CFireballSystem::Animate(float fTimeElapsed)
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

        // 중력 + 상승력 (수명 초반엔 상승, 후반엔 낙하)
        float lifeRatio = p.lifetime / p.maxLifetime;
        p.velocity.y -= 2.0f * fTimeElapsed;
        p.velocity.y += 4.0f * (1.0f - lifeRatio) * fTimeElapsed;

        // 난류: 시간이 갈수록 흔들림 증가
        float turb = lifeRatio * 3.0f;
        p.velocity.x += sinf(p.lifetime * 7.3f + p.position.x) * turb * fTimeElapsed;
        p.velocity.z += cosf(p.lifetime * 6.1f + p.position.z) * turb * fTimeElapsed;

        // 스프라이트 시트 UV 스크롤
        // 4x4 시트에서 16프레임, 1초에 8프레임 재생 → V 좌표 기준
        p.uvOffset = (p.lifetime * 8.0f / 16.0f); // 0~0.5 범위 (행 단위)

        m_bNeedUpload = true;
    }
}

// -------------------------------------------------------
void CFireballSystem::UploadToGPU(ID3D12GraphicsCommandList* pd3dCommandList)
{
    if (!m_bNeedUpload) return;
    m_bNeedUpload = false;

    memcpy(m_pMappedData, m_Particles, sizeof(m_Particles));

    // GENERIC_READ → COPY_DEST
    auto toCopy = CD3DX12_RESOURCE_BARRIER::Transition(
        m_pParticleDefaultBuffer,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_COPY_DEST);
    pd3dCommandList->ResourceBarrier(1, &toCopy);

    pd3dCommandList->CopyBufferRegion(
        m_pParticleDefaultBuffer, 0,
        m_pParticleUploadBuffer, 0,
        sizeof(m_Particles));

    // COPY_DEST → GENERIC_READ (SRV 읽기 가능)
    auto toRead = CD3DX12_RESOURCE_BARRIER::Transition(
        m_pParticleDefaultBuffer,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ);
    pd3dCommandList->ResourceBarrier(1, &toRead);
}

// -------------------------------------------------------
void CFireballSystem::Render(
    ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    UploadToGPU(pd3dCommandList);

    // 파이프라인 / 디스크립터 설정
    m_pShader->Render(pd3dCommandList, pCamera);

    if (m_pTexture)
        m_pTexture->UpdateShaderVariable(pd3dCommandList, 0, 0);

    pd3dCommandList->SetGraphicsRootShaderResourceView(19, m_pParticleDefaultBuffer->GetGPUVirtualAddress());

    // 쿼드(6정점) × MAX_PARTICLES 인스턴스 – 단일 드로우콜
    pd3dCommandList->IASetVertexBuffers(0, 1, &m_QuadVBView);
    pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dCommandList->DrawInstanced(6, MAX_PARTICLES, 0, 0);
    ++g_nDrawCallCount;
}

// -------------------------------------------------------
CFireballSystem::~CFireballSystem()
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

void CFireballSystem::ReleaseUploadBuffers()
{
    // 업로드 힙은 매 프레임 사용하므로 해제하지 않음
    // (쿼드 VB는 업로드 힙이지만 변하지 않으므로 그대로 유지)
}

std::vector<std::pair<int, XMFLOAT3>> CFireballSystem::GetActiveParticles() const
{
    std::vector<std::pair<int, XMFLOAT3>> result;
    for (int i = 0; i < MAX_PARTICLES; ++i)
        if (m_Particles[i].active)
            result.push_back({ i, m_Particles[i].position });
    return result;
}
