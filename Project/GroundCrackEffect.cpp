#include "stdafx.h"
#include "GroundCrackEffect.h"
#include "Camera.h"
#include "Common.h"

CGroundCrackEffect::~CGroundCrackEffect()
{
    if (m_pVertexUploadBuffer)
    {
        m_pVertexUploadBuffer->Unmap(0, nullptr);
        m_pVertexUploadBuffer->Release();
    }
    if (m_pIndexBuffer)   m_pIndexBuffer->Release();
    if (m_pPipelineState) m_pPipelineState->Release();
}

void CGroundCrackEffect::Create(ID3D12Device* pd3dDevice,
    ID3D12GraphicsCommandList* pd3dCommandList,
    ID3D12RootSignature* pd3dRootSignature)
{
    // ──────────────────────────────────────────────────────────
    // 1) 동적 버텍스 버퍼 (Upload Heap, 영구 매핑)
    // ──────────────────────────────────────────────────────────
    {
        UINT64 bufSize = sizeof(GroundCrackVertex) * MAX_VERTS;

        D3D12_HEAP_PROPERTIES heapProp = {};
        heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width = bufSize;
        resDesc.Height = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.Format = DXGI_FORMAT_UNKNOWN;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        pd3dDevice->CreateCommittedResource(
            &heapProp, D3D12_HEAP_FLAG_NONE,
            &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&m_pVertexUploadBuffer));

        D3D12_RANGE readRange = { 0, 0 };
        m_pVertexUploadBuffer->Map(0, &readRange, (void**)&m_pMappedVerts);

        m_VBView.BufferLocation = m_pVertexUploadBuffer->GetGPUVirtualAddress();
        m_VBView.SizeInBytes = static_cast<UINT>(bufSize);
        m_VBView.StrideInBytes = sizeof(GroundCrackVertex);
    }

    // ──────────────────────────────────────────────────────────
    // 2) 정적 인덱스 버퍼 (Upload Heap 직접 사용)
    //    192 bytes 짜리 소형 버퍼이므로 CopyResource 없이
    //    Upload Heap에 바로 써도 성능 차이 없음.
    //    → 임시 버퍼 Release 타이밍 문제(Device Removed) 원천 차단
    // ──────────────────────────────────────────────────────────
    {
        UINT16 indices[MAX_INDICES];
        for (int seg = 0; seg < TOTAL_CRACKS; ++seg)
        {
            UINT16 base = static_cast<UINT16>(seg * VERTS_PER_SEG);
            int    ib = seg * INDEX_PER_SEG;
            indices[ib + 0] = base + 0;
            indices[ib + 1] = base + 2;
            indices[ib + 2] = base + 1;
            indices[ib + 3] = base + 1;
            indices[ib + 4] = base + 2;
            indices[ib + 5] = base + 3;
        }

        UINT64 ibSize = sizeof(indices);

        D3D12_HEAP_PROPERTIES upHeap = {};
        upHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC ibDesc = {};
        ibDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        ibDesc.Width = ibSize;
        ibDesc.Height = 1;
        ibDesc.DepthOrArraySize = 1;
        ibDesc.MipLevels = 1;
        ibDesc.Format = DXGI_FORMAT_UNKNOWN;
        ibDesc.SampleDesc.Count = 1;
        ibDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        pd3dDevice->CreateCommittedResource(
            &upHeap, D3D12_HEAP_FLAG_NONE,
            &ibDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&m_pIndexBuffer));

        void* pMapped = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        m_pIndexBuffer->Map(0, &readRange, &pMapped);
        memcpy(pMapped, indices, ibSize);
        m_pIndexBuffer->Unmap(0, nullptr);

        m_IBView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
        m_IBView.SizeInBytes = static_cast<UINT>(ibSize);
        m_IBView.Format = DXGI_FORMAT_R16_UINT;
    }

    // ──────────────────────────────────────────────────────────
    // 3) PSO 생성 (가산 블렌딩 + Depth Write Off)
    // ──────────────────────────────────────────────────────────
    {
        ID3DBlob* pVSBlob = nullptr;
        ID3DBlob* pPSBlob = nullptr;
        ID3DBlob* pErrBlob = nullptr;

        UINT compileFlags = 0;
#if defined(_DEBUG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        D3DCompileFromFile(L"Shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "VSGroundCrack", "vs_5_1", compileFlags, 0, &pVSBlob, &pErrBlob);
        if (pErrBlob) { OutputDebugStringA((char*)pErrBlob->GetBufferPointer()); pErrBlob->Release(); pErrBlob = nullptr; }

        D3DCompileFromFile(L"Shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "PSGroundCrack", "ps_5_1", compileFlags, 0, &pPSBlob, &pErrBlob);
        if (pErrBlob) { OutputDebugStringA((char*)pErrBlob->GetBufferPointer()); pErrBlob->Release(); pErrBlob = nullptr; }

        // 컴파일 실패 시 PSO 생성 스킵 (nullptr 역참조 방지)
        if (!pVSBlob || !pPSBlob)
        {
            if (pVSBlob) pVSBlob->Release();
            if (pPSBlob) pPSBlob->Release();
            return;
        }

        D3D12_INPUT_ELEMENT_DESC inputElems[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32_FLOAT,        0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_RASTERIZER_DESC rsDesc = {};
        rsDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rsDesc.CullMode = D3D12_CULL_MODE_NONE;
        rsDesc.FrontCounterClockwise = FALSE;
        rsDesc.DepthClipEnable = TRUE;

        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;  // Additive
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        dsDesc.StencilEnable = FALSE;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = pd3dRootSignature;
        psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
        psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
        psoDesc.RasterizerState = rsDesc;
        psoDesc.BlendState = blendDesc;
        psoDesc.DepthStencilState = dsDesc;
        psoDesc.InputLayout = { inputElems, _countof(inputElems) };
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState));
        if (FAILED(hr))
            OutputDebugStringA("[GroundCrackEffect] PSO 생성 실패\n");

        pVSBlob->Release();
        pPSBlob->Release();
    }
}

void CGroundCrackEffect::Trigger(const XMFLOAT3& playerPos, const XMFLOAT3& playerLook)
{
    m_xmf3Origin = playerPos;
    m_fTimer = 0.0f;
    m_bActive = true;

    InitCrackSegments(playerPos, playerLook);
}

void CGroundCrackEffect::InitCrackSegments(const XMFLOAT3& origin, const XMFLOAT3& look)
{
    float baseAngle = atan2f(look.x, look.z);

    for (int i = 0; i < MAIN_CRACK_COUNT; ++i)
    {
        float angle = baseAngle + (XM_2PI / MAIN_CRACK_COUNT) * i;
        float jitter = (i % 2 == 0) ? 0.14f : -0.14f;
        angle += jitter;

        XMFLOAT3 dir = { sinf(angle), 0.0f, cosf(angle) };
        XMFLOAT3 perp = { -dir.z, 0.0f, dir.x };

        CrackSegment& seg = m_Cracks[i];
        seg.dir = dir;
        seg.perp = perp;
        seg.maxLen = (i % 2 == 0) ? 3.2f : 2.4f;
        seg.halfWidth = (i % 2 == 0) ? 0.18f : 0.12f;
        seg.startT = 0.0f;
    }

    for (int i = 0; i < BRANCH_CRACK_COUNT; ++i)
    {
        CrackSegment& main = m_Cracks[i];

        float baseDir = atan2f(main.dir.x, main.dir.z);
        float branchAngle = baseDir + ((i % 2 == 0) ? 0.52f : -0.52f);

        XMFLOAT3 dir = { sinf(branchAngle), 0.0f, cosf(branchAngle) };
        XMFLOAT3 perp = { -dir.z, 0.0f, dir.x };

        CrackSegment& seg = m_Cracks[MAIN_CRACK_COUNT + i];
        seg.dir = dir;
        seg.perp = perp;
        seg.maxLen = main.maxLen * 0.55f;
        seg.halfWidth = main.halfWidth * 0.65f;
        seg.startT = 0.12f;
    }
}

void CGroundCrackEffect::Update(float fTimeElapsed)
{
    if (!m_bActive) return;

    m_fTimer += fTimeElapsed;
    if (m_fTimer >= m_fDuration)
    {
        m_fTimer = 0.0f;
        m_bActive = false;
        return;
    }

    UpdateVertices(m_fTimer / m_fDuration);
}

void CGroundCrackEffect::UpdateVertices(float t)
{
    float fadeIn = min(t / 0.15f, 1.0f);
    float fadeOut = (t > 0.65f) ? 1.0f - (t - 0.65f) / 0.35f : 1.0f;
    float globalAlpha = fadeIn * fadeOut;

    float expansion = min(t / 0.40f, 1.0f);
    expansion = 1.0f - (1.0f - expansion) * (1.0f - expansion); // ease-out

    XMFLOAT3 Y_OFFSET = { 0.0f, 0.02f, 0.0f };

    for (int seg = 0; seg < TOTAL_CRACKS; ++seg)
    {
        CrackSegment& crack = m_Cracks[seg];
        int vBase = seg * VERTS_PER_SEG;

        // 보조 균열은 startT 이후부터 등장
        float localExp = max(0.0f, expansion - crack.startT) / (1.0f - crack.startT);
        localExp = min(localExp, 1.0f);
        float curLen = crack.maxLen * localExp;

        // 보조 균열 시작점 = 주 균열 50% 지점
        XMFLOAT3 segOrigin = m_xmf3Origin;
        if (seg >= MAIN_CRACK_COUNT)
        {
            CrackSegment& mainCrack = m_Cracks[seg - MAIN_CRACK_COUNT];
            float branchStart = mainCrack.maxLen * 0.50f;
            segOrigin.x += mainCrack.dir.x * branchStart;
            segOrigin.z += mainCrack.dir.z * branchStart;
        }

        float tipHW = crack.halfWidth * 0.25f; // 끝부분 폭 (쐐기형)

        // v0: 왼쪽-뿌리
        m_pMappedVerts[vBase + 0].position = {
            segOrigin.x - crack.perp.x * crack.halfWidth + Y_OFFSET.x,
            segOrigin.y + Y_OFFSET.y,
            segOrigin.z - crack.perp.z * crack.halfWidth + Y_OFFSET.z };
        m_pMappedVerts[vBase + 0].uv = { 0.0f, 0.0f };

        // v1: 오른쪽-뿌리
        m_pMappedVerts[vBase + 1].position = {
            segOrigin.x + crack.perp.x * crack.halfWidth + Y_OFFSET.x,
            segOrigin.y + Y_OFFSET.y,
            segOrigin.z + crack.perp.z * crack.halfWidth + Y_OFFSET.z };
        m_pMappedVerts[vBase + 1].uv = { 0.0f, 1.0f };

        // v2: 왼쪽-끝
        m_pMappedVerts[vBase + 2].position = {
            segOrigin.x + crack.dir.x * curLen - crack.perp.x * tipHW + Y_OFFSET.x,
            segOrigin.y + Y_OFFSET.y,
            segOrigin.z + crack.dir.z * curLen - crack.perp.z * tipHW + Y_OFFSET.z };
        m_pMappedVerts[vBase + 2].uv = { 1.0f, 0.0f };

        // v3: 오른쪽-끝
        m_pMappedVerts[vBase + 3].position = {
            segOrigin.x + crack.dir.x * curLen + crack.perp.x * tipHW + Y_OFFSET.x,
            segOrigin.y + Y_OFFSET.y,
            segOrigin.z + crack.dir.z * curLen + crack.perp.z * tipHW + Y_OFFSET.z };
        m_pMappedVerts[vBase + 3].uv = { 1.0f, 1.0f };

        // 4개 버텍스 모두 동일한 alpha (PS에서 UV 기반으로 가장자리 페이드 처리)
        float segAlpha = globalAlpha * localExp;
        for (int v = 0; v < VERTS_PER_SEG; ++v)
            m_pMappedVerts[vBase + v].alpha = segAlpha;
    }
}

void CGroundCrackEffect::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    if (!m_bActive || !m_pPipelineState) return;

    pd3dCommandList->SetPipelineState(m_pPipelineState);
    pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dCommandList->IASetVertexBuffers(0, 1, &m_VBView);
    pd3dCommandList->IASetIndexBuffer(&m_IBView);
    pd3dCommandList->DrawIndexedInstanced(MAX_INDICES, 1, 0, 0, 0);
    ++g_nDrawCallCount;
}