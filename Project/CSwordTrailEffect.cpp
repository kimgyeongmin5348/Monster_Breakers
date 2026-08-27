#include "stdafx.h"
#include "CSwordTrailEffect.h"
#include "Camera.h"
#include "Common.h"

CSwordTrailEffect::~CSwordTrailEffect()
{
    if (m_pVertexUploadBuffer)
    {
        m_pVertexUploadBuffer->Unmap(0, nullptr);
        m_pVertexUploadBuffer->Release();
    }
    if (m_pIndexBuffer)   m_pIndexBuffer->Release();
    if (m_pPipelineState) m_pPipelineState->Release();
}

void CSwordTrailEffect::Create(ID3D12Device* pd3dDevice,
    ID3D12GraphicsCommandList* pd3dCommandList,
    ID3D12RootSignature* pd3dRootSignature)
{
    // ------------------------------------------------------------
    // 1) Dynamic vertex buffer (upload heap, permanently mapped) - rebuilt every frame.
    // ------------------------------------------------------------
    {
        UINT64 bufSize = sizeof(SwordTrailVertex) * MAX_VERTS;

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
        m_VBView.StrideInBytes = sizeof(SwordTrailVertex);
    }

    // ------------------------------------------------------------
    // 2) Static index buffer: a strip of quads connecting ring i to ring i+1.
    // ------------------------------------------------------------
    {
        UINT16 indices[MAX_INDICES];
        for (int seg = 0; seg < SEGMENTS; ++seg)
        {
            UINT16 base = static_cast<UINT16>(seg * VERTS_PER_RING);
            UINT16 next = static_cast<UINT16>((seg + 1) * VERTS_PER_RING);
            int    ib = seg * 6;

            indices[ib + 0] = base + 0; // inner_i
            indices[ib + 1] = next + 0; // inner_i+1
            indices[ib + 2] = base + 1; // outer_i
            indices[ib + 3] = base + 1; // outer_i
            indices[ib + 4] = next + 0; // inner_i+1
            indices[ib + 5] = next + 1; // outer_i+1
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

    // ------------------------------------------------------------
    // 3) PSO (additive blend, depth write off) - same recipe as CGroundCrackEffect.
    // ------------------------------------------------------------
    {
        ID3DBlob* pVSBlob = nullptr;
        ID3DBlob* pPSBlob = nullptr;
        ID3DBlob* pErrBlob = nullptr;

        UINT compileFlags = 0;
#if defined(_DEBUG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        D3DCompileFromFile(L"Shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "VSSwordTrail", "vs_5_1", compileFlags, 0, &pVSBlob, &pErrBlob);
        if (pErrBlob) { OutputDebugStringA((char*)pErrBlob->GetBufferPointer()); pErrBlob->Release(); pErrBlob = nullptr; }

        D3DCompileFromFile(L"Shaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "PSSwordTrail", "ps_5_1", compileFlags, 0, &pPSBlob, &pErrBlob);
        if (pErrBlob) { OutputDebugStringA((char*)pErrBlob->GetBufferPointer()); pErrBlob->Release(); pErrBlob = nullptr; }

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
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;  // additive glow
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
            OutputDebugStringA("[SwordTrailEffect] PSO creation failed\n");

        pVSBlob->Release();
        pPSBlob->Release();
    }
}

void CSwordTrailEffect::Trigger(const XMFLOAT3& origin, const XMFLOAT3& forward, const XMFLOAT3& right,
    float radius, float arcDegrees, float delaySeconds)
{
    if (delaySeconds > 0.0f)
    {
        m_xmf3PendingOrigin = origin;
        m_xmf3PendingForward = forward;
        m_xmf3PendingRight = right;
        m_fPendingRadius = radius;
        m_fPendingArcDegrees = arcDegrees;
        m_fPendingDelay = delaySeconds;
        m_bPending = true;
        return;
    }

    m_xmf3Origin = origin;
    m_xmf3Forward = forward;
    m_xmf3Right = right;
    m_fRadius = radius;
    m_fArcRadians = XMConvertToRadians(arcDegrees);

    m_bPending = false;
    m_fTimer = 0.0f;
    m_bActive = true;

    UpdateVertices(0.0f);
}

void CSwordTrailEffect::Update(float fTimeElapsed)
{
    if (m_bPending)
    {
        m_fPendingDelay -= fTimeElapsed;
        if (m_fPendingDelay <= 0.0f)
        {
            m_bPending = false;
            Trigger(m_xmf3PendingOrigin, m_xmf3PendingForward, m_xmf3PendingRight,
                m_fPendingRadius, m_fPendingArcDegrees, 0.0f);
        }
    }

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

// The ring is revealed progressively from tail to head (like a blade actively
// cutting through the arc), with a short comet-tail glow trailing the moving
// head, and the whole thing fades out once the sweep completes.
void CSwordTrailEffect::UpdateVertices(float t)
{
    const float SWEEP_FRAC = 0.55f;   // fraction of the duration spent sweeping the arc
    const float TRAIL_FRAC = 0.30f;   // trailing glow length, as a fraction of the arc
    const float FADEOUT_START = 0.55f;  // fraction of duration when the lingering trail starts fading out
    const float HALF_WIDTH = 0.28f;   // ribbon width (world units)
    const float BOB_HEIGHT = 0.35f;   // vertical arc of the swing (rises then falls)

    float sweepT = min(t / SWEEP_FRAC, 1.0f);
    sweepT = 1.0f - (1.0f - sweepT) * (1.0f - sweepT); // ease-out

    float globalFade = (t < FADEOUT_START) ? 1.0f : max(0.0f, 1.0f - (t - FADEOUT_START) / (1.0f - FADEOUT_START));

    XMVECTOR vOrigin = XMLoadFloat3(&m_xmf3Origin);
    XMVECTOR vForward = XMLoadFloat3(&m_xmf3Forward);
    XMVECTOR vRight = XMLoadFloat3(&m_xmf3Right);

    for (int i = 0; i <= SEGMENTS; ++i)
    {
        float segFrac = (float)i / (float)SEGMENTS;
        float angle = -m_fArcRadians * 0.5f + m_fArcRadians * segFrac;

        XMVECTOR vDir = XMVector3Normalize(vForward * cosf(angle) + vRight * sinf(angle));
        XMVECTOR vCenter = vOrigin + vDir * m_fRadius;

        float bob = sinf(segFrac * XM_PI) * BOB_HEIGHT;
        vCenter = XMVectorSetY(vCenter, XMVectorGetY(vCenter) + bob);

        XMVECTOR vInner = vCenter - vDir * HALF_WIDTH;
        XMVECTOR vOuter = vCenter + vDir * HALF_WIDTH;

        int vBase = i * VERTS_PER_RING;
        XMStoreFloat3(&m_pMappedVerts[vBase + 0].position, vInner);
        XMStoreFloat3(&m_pMappedVerts[vBase + 1].position, vOuter);

        m_pMappedVerts[vBase + 0].uv = { segFrac, 0.0f };
        m_pMappedVerts[vBase + 1].uv = { segFrac, 1.0f };

        float behind = sweepT - segFrac;
        float alpha;
        if (segFrac > sweepT)
            alpha = 0.0f; // the blade hasn't reached this part of the arc yet
        else
            alpha = max(0.0f, 1.0f - behind / TRAIL_FRAC) * globalFade;

        m_pMappedVerts[vBase + 0].alpha = alpha;
        m_pMappedVerts[vBase + 1].alpha = alpha;
    }
}

void CSwordTrailEffect::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    if (!m_bActive || !m_pPipelineState) return;

    pd3dCommandList->SetPipelineState(m_pPipelineState);
    pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dCommandList->IASetVertexBuffers(0, 1, &m_VBView);
    pd3dCommandList->IASetIndexBuffer(&m_IBView);
    pd3dCommandList->DrawIndexedInstanced(MAX_INDICES, 1, 0, 0, 0);
    ++g_nDrawCallCount;
}
