#include "stdafx.h"
#include "CAttackRangeEffect.h"

namespace
{
    constexpr int TERRAIN_RANGE_GRID_CELLS = 24;
    constexpr float TERRAIN_RANGE_Y_OFFSET = 0.05f;

    // 기존 사각형 2개의 정점만 있는 평면은 경사진 지형을 관통한다.
    // 범위를 격자로 나눈 후 각 정점의 Y를 HeightMap에서 샘플링해
    // 경사와 높낮이를 따라가는 공격 범위를 만든다.
    class CTerrainRangeMesh final : public CMesh
    {
    public:
        explicit CTerrainRangeMesh(ID3D12Device* pd3dDevice)
            : CMesh(pd3dDevice, nullptr)
        {
            m_nVertices = TERRAIN_RANGE_GRID_CELLS * TERRAIN_RANGE_GRID_CELLS * 6;
            m_nStride = sizeof(CTexturedVertex);
            m_nOffset = 0;
            m_nSlot = 0;
            m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Width = static_cast<UINT64>(m_nStride) * m_nVertices;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            const HRESULT hr = pd3dDevice->CreateCommittedResource(
                &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                __uuidof(ID3D12Resource), reinterpret_cast<void**>(&m_pd3dPositionBuffer));

            if (SUCCEEDED(hr))
            {
                D3D12_RANGE readRange = { 0, 0 };
                m_pd3dPositionBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pMappedVertices));

                m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
                m_d3dPositionBufferView.StrideInBytes = m_nStride;
                m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;
            }
        }

        ~CTerrainRangeMesh() override
        {
            if (m_pd3dPositionBuffer && m_pMappedVertices)
            {
                m_pd3dPositionBuffer->Unmap(0, nullptr);
                m_pMappedVertices = nullptr;
            }
        }

        void UpdateForTerrain(const XMFLOAT3& center, float radius, CHeightMapTerrain* pTerrain)
        {
            if (!m_pMappedVertices) return;

            constexpr int VERTICES_PER_SIDE = TERRAIN_RANGE_GRID_CELLS + 1;
            XMFLOAT3 positions[VERTICES_PER_SIDE * VERTICES_PER_SIDE];

            const XMFLOAT3 terrainOrigin = pTerrain ? pTerrain->GetPosition() : XMFLOAT3(0.0f, 0.0f, 0.0f);
            const XMFLOAT3 terrainScale = pTerrain ? pTerrain->GetScale() : XMFLOAT3(1.0f, 1.0f, 1.0f);
            const float terrainWidth = pTerrain ? (pTerrain->GetHeightMapWidth() - 1) * terrainScale.x : 0.0f;
            const float terrainLength = pTerrain ? (pTerrain->GetHeightMapLength() - 1) * terrainScale.z : 0.0f;

            for (int z = 0; z < VERTICES_PER_SIDE; ++z)
            {
                const float v = static_cast<float>(z) / TERRAIN_RANGE_GRID_CELLS;
                const float worldZ = center.z + ((v * 2.0f) - 1.0f) * radius;

                for (int x = 0; x < VERTICES_PER_SIDE; ++x)
                {
                    const float u = static_cast<float>(x) / TERRAIN_RANGE_GRID_CELLS;
                    const float worldX = center.x + ((u * 2.0f) - 1.0f) * radius;
                    float worldY = center.y + TERRAIN_RANGE_Y_OFFSET;

                    if (pTerrain)
                    {
                        const float localX = worldX - terrainOrigin.x;
                        const float localZ = worldZ - terrainOrigin.z;
                        if (localX >= 0.0f && localZ >= 0.0f && localX < terrainWidth && localZ < terrainLength)
                        {
                            const int terrainZ = static_cast<int>(localZ / terrainScale.z);
                            const bool reverseQuad = (terrainZ % 2) != 0;
                            worldY = pTerrain->GetHeight(localX, localZ, reverseQuad)
                                + terrainOrigin.y + TERRAIN_RANGE_Y_OFFSET;
                        }
                    }

                    positions[z * VERTICES_PER_SIDE + x] = XMFLOAT3(worldX, worldY, worldZ);
                }
            }

            int out = 0;
            for (int z = 0; z < TERRAIN_RANGE_GRID_CELLS; ++z)
            {
                const float v0 = static_cast<float>(z) / TERRAIN_RANGE_GRID_CELLS;
                const float v1 = static_cast<float>(z + 1) / TERRAIN_RANGE_GRID_CELLS;
                for (int x = 0; x < TERRAIN_RANGE_GRID_CELLS; ++x)
                {
                    const float u0 = static_cast<float>(x) / TERRAIN_RANGE_GRID_CELLS;
                    const float u1 = static_cast<float>(x + 1) / TERRAIN_RANGE_GRID_CELLS;
                    const XMFLOAT3& p00 = positions[z * VERTICES_PER_SIDE + x];
                    const XMFLOAT3& p10 = positions[z * VERTICES_PER_SIDE + x + 1];
                    const XMFLOAT3& p01 = positions[(z + 1) * VERTICES_PER_SIDE + x];
                    const XMFLOAT3& p11 = positions[(z + 1) * VERTICES_PER_SIDE + x + 1];

                    m_pMappedVertices[out++] = CTexturedVertex(p00, XMFLOAT2(u0, v0));
                    m_pMappedVertices[out++] = CTexturedVertex(p01, XMFLOAT2(u0, v1));
                    m_pMappedVertices[out++] = CTexturedVertex(p11, XMFLOAT2(u1, v1));
                    m_pMappedVertices[out++] = CTexturedVertex(p00, XMFLOAT2(u0, v0));
                    m_pMappedVertices[out++] = CTexturedVertex(p11, XMFLOAT2(u1, v1));
                    m_pMappedVertices[out++] = CTexturedVertex(p10, XMFLOAT2(u1, v0));
                }
            }
        }

    private:
        CTexturedVertex* m_pMappedVertices = nullptr;
    };
}

CGroundAttackRangeEffect::CGroundAttackRangeEffect()
{}

CGroundAttackRangeEffect::~CGroundAttackRangeEffect()
{
    Release();
}

void CGroundAttackRangeEffect::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
    ID3D12RootSignature* pd3dGraphicsRootSignature, int nPoolSize)
{
    m_vIndicators.clear();
    m_vIndicators.resize(nPoolSize);

    for (int i = 0; i < nPoolSize; ++i)
    {
        // -1..1 범위의 평평한 사각 메쉬. PlaceFlatOnGround에서 SetScale(radius)로 실제 반지름을 맞춘다.
        CMesh* pMesh = new CTerrainRangeMesh(pd3dDevice);

        CGameObject* pObject = new CGameObject(1);
        pObject->SetMesh(pMesh);

        CMaterial* pMaterial = new CMaterial(0);
        pMaterial->SetShader(CMaterial::m_pGroundRangeShader);
        pMaterial->m_xmf4AlbedoColor = XMFLOAT4(1.0f, 0.15f, 0.05f, 0.0f); // 처음엔 안 보이게(alpha=0)
        pMaterial->m_xmf4SpecularColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
        pMaterial->m_xmf4EmissiveColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // x=부채꼴여부, y=정면각, z=half-angle
        pObject->SetMaterial(0, pMaterial);

        pObject->SetVisible(false);

        m_vIndicators[i].pObject = pObject;
        m_vIndicators[i].bActive = false;
    }

    m_nNextIndex = 0;
}

void CGroundAttackRangeEffect::Release()
{
    for (auto& ind : m_vIndicators)
    {
        if (ind.pObject) ind.pObject->Release();
        ind.pObject = nullptr;
    }
    m_vIndicators.clear();
}

void CGroundAttackRangeEffect::ReleaseUploadBuffers()
{
    for (auto& ind : m_vIndicators)
        if (ind.pObject) ind.pObject->ReleaseUploadBuffers();
}

// 지형을 샘플링한 월드 좌표 격자로 갱신한다.
void CGroundAttackRangeEffect::PlaceOnGround(CGameObject* pObject, const XMFLOAT3& xmf3Center, float fRadius)
{
    const float visualRadius = fRadius * 1.5f;
    auto* pTerrainMesh = static_cast<CTerrainRangeMesh*>(pObject->m_pMesh);
    if (pTerrainMesh) pTerrainMesh->UpdateForTerrain(xmf3Center, visualRadius, m_pTerrain);

    // 메시 정점이 이미 월드 좌표이므로 오브젝트 행렬은 단위 행렬을 쓴다.
    pObject->m_xmf4x4ToParent = Matrix4x4::Identity();
    pObject->UpdateTransform(NULL);
}

// 격자 UV의 +X/+Y는 월드 +X/+Z와 같은 방향이다.
float CGroundAttackRangeEffect::WorldDirectionToLocalAngle(const XMFLOAT3 & xmf3Direction)
{
    float dx = xmf3Direction.x;
    float dz = xmf3Direction.z;
    float lenSq = dx * dx + dz * dz;
    if (lenSq < 1e-8f) return 0.0f; // 방향이 없으면 기본값(로컬 +X)

    return atan2f(dx, dz);
}

int CGroundAttackRangeEffect::AcquireSlot()
{
    if (m_vIndicators.empty()) return -1;

    // 비어있는(비활성) 슬롯을 우선 찾고, 없으면 라운드로빈으로 가장 오래된 것을 덮어쓴다.
    for (size_t i = 0; i < m_vIndicators.size(); ++i)
    {
        if (!m_vIndicators[i].bActive) return (int)i;
    }

    int useIndex = m_nNextIndex;
    m_nNextIndex = (m_nNextIndex + 1) % (int)m_vIndicators.size();
    return useIndex;
}

void CGroundAttackRangeEffect::Spawn(const XMFLOAT3& xmf3Center, float fRadius, float fWarmupTime, const XMFLOAT4& xmf4Color)
{
    int useIndex = AcquireSlot();
    if (useIndex < 0) return;

    Indicator& ind = m_vIndicators[useIndex];
    if (!ind.pObject) return;

    ind.bActive = true;
    ind.fTimer = 0.0f;
    ind.fWarmupTime = max(0.05f, fWarmupTime);
    ind.xmf3Color = XMFLOAT3(xmf4Color.x, xmf4Color.y, xmf4Color.z);
    ind.bSector = false; // 원형
    ind.fFacingAngle = 0.0f;
    ind.fHalfAngle = XM_PI;

    PlaceOnGround(ind.pObject, xmf3Center, fRadius);
    ind.pObject->SetVisible(true);
}

void CGroundAttackRangeEffect::Spawn(const XMFLOAT3& xmf3Center, float fRadius, float fWarmupTime,
    const XMFLOAT3& xmf3Direction, float fHalfAngleDeg, const XMFLOAT4& xmf4Color)
{
    int useIndex = AcquireSlot();
    if (useIndex < 0) return;

    Indicator& ind = m_vIndicators[useIndex];
    if (!ind.pObject) return;

    ind.bActive = true;
    ind.fTimer = 0.0f;
    ind.fWarmupTime = max(0.05f, fWarmupTime);
    ind.xmf3Color = XMFLOAT3(xmf4Color.x, xmf4Color.y, xmf4Color.z);
    ind.bSector = true;
    ind.fFacingAngle = WorldDirectionToLocalAngle(xmf3Direction);
    ind.fHalfAngle = XMConvertToRadians(max(1.0f, fHalfAngleDeg));

    PlaceOnGround(ind.pObject, xmf3Center, fRadius);
    ind.pObject->SetVisible(true);
}

void CGroundAttackRangeEffect::Animate(float fTimeElapsed)
{
    for (auto& ind : m_vIndicators)
    {
        if (!ind.bActive || !ind.pObject) continue;

        ind.fTimer += fTimeElapsed;
        float fTotal = ind.fWarmupTime + ind.fFadeOutTime;

        if (ind.fTimer >= fTotal)
        {
            ind.bActive = false;
            ind.pObject->SetVisible(false);
            continue;
        }

        float progress, fade;
        if (ind.fTimer < ind.fWarmupTime)
        {
            progress = ind.fTimer / ind.fWarmupTime;       // 0 -> 1로 차오름
            fade = min(1.0f, progress * 3.0f);             // 등장 시 살짝 페이드인
        }
        else
        {
            progress = 1.0f;
            float t = (ind.fTimer - ind.fWarmupTime) / ind.fFadeOutTime;
            fade = 1.0f - t;                               // 타격 순간 -> 빠르게 페이드아웃
        }

        CMaterial* pMat = ind.pObject->GetMaterial(0);
        if (pMat)
        {
            pMat->m_xmf4AlbedoColor = XMFLOAT4(ind.xmf3Color.x, ind.xmf3Color.y, ind.xmf3Color.z, fade);
            pMat->m_xmf4SpecularColor = XMFLOAT4(0.0f, 0.0f, 0.0f, progress);

            // 부채꼴 파라미터를 emissive 채널에 실어서 PSGroundRange로 전달
            // x = 부채꼴 모드(1)/원형(0), y = 정면각(라디안), z = half-angle(라디안)
            pMat->m_xmf4EmissiveColor = XMFLOAT4(
                ind.bSector ? 1.0f : 0.0f,
                ind.fFacingAngle,
                ind.fHalfAngle,
                0.0f);
        }
    }
}

void CGroundAttackRangeEffect::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    for (auto& ind : m_vIndicators)
    {
        if (ind.bActive && ind.pObject && ind.pObject->GetVisible())
            ind.pObject->Render(pd3dCommandList, pCamera);
    }
}
