#include "stdafx.h"
#include "CBeamSystem.h"
#include "d3dx12.h"
#include "Common.h"

class CBeamShader : public CShader
{
public:
    virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override
    {
        D3D12_INPUT_ELEMENT_DESC* pInputElementDescs =
            new D3D12_INPUT_ELEMENT_DESC[2];

        pInputElementDescs[0] =
        {
            "POSITION", 0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        };

        pInputElementDescs[1] =
        {
            "TEXCOORD", 0,
            DXGI_FORMAT_R32G32_FLOAT,
            0, 12,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        };

        D3D12_INPUT_LAYOUT_DESC desc{};
        desc.pInputElementDescs = pInputElementDescs;
        desc.NumElements = 2;
        return desc;
    }

    virtual D3D12_RASTERIZER_DESC CreateRasterizerState() override
    {
        D3D12_RASTERIZER_DESC desc = CShader::CreateRasterizerState();
        desc.CullMode = D3D12_CULL_MODE_NONE;
        return desc;
    }

    virtual D3D12_BLEND_DESC CreateBlendState() override
    {
        D3D12_BLEND_DESC desc{};
        ZeroMemory(&desc, sizeof(D3D12_BLEND_DESC));

        desc.AlphaToCoverageEnable = FALSE;
        desc.IndependentBlendEnable = FALSE;

        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].LogicOpEnable = FALSE;
        desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        return desc;
    }

    virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState() override
    {
        D3D12_DEPTH_STENCIL_DESC desc{};
        ZeroMemory(&desc, sizeof(D3D12_DEPTH_STENCIL_DESC));

        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        desc.StencilEnable = FALSE;

        return desc;
    }

    virtual D3D12_SHADER_BYTECODE CreateVertexShader() override
    {
        return CompileShaderFromFile(
            L"Shaders.hlsl",
            "VSBeam",
            "vs_5_1",
            &m_pd3dVertexShaderBlob
        );
    }

    virtual D3D12_SHADER_BYTECODE CreatePixelShader() override
    {
        return CompileShaderFromFile(
            L"Shaders.hlsl",
            "PSBeam",
            "ps_5_1",
            &m_pd3dPixelShaderBlob
        );
    }
};

CBeamSystem::CBeamSystem()
{}

CBeamSystem::~CBeamSystem()
{
    if (m_pVertexUploadBuffer)
    {
        m_pVertexUploadBuffer->Unmap(0, nullptr);
        m_pVertexUploadBuffer->Release();
        m_pVertexUploadBuffer = nullptr;
    }

    if (m_pShader)
    {
        m_pShader->Release();
        m_pShader = nullptr;
    }
}

void CBeamSystem::Create(
    ID3D12Device* pd3dDevice,
    ID3D12GraphicsCommandList* pd3dCommandList,
    ID3D12RootSignature* pd3dRootSignature)
{
    CreateVertexBuffer(pd3dDevice);

    m_pShader = new CBeamShader();
    m_pShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dRootSignature);
    m_pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CBeamSystem::CreateVertexBuffer(ID3D12Device* pd3dDevice)
{
    UINT64 bufferSize = sizeof(BeamVertex) * 6;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc =
        CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    pd3dDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pVertexUploadBuffer)
    );

    m_pVertexUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pMappedVertices));

    m_VertexBufferView.BufferLocation =
        m_pVertexUploadBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.StrideInBytes = sizeof(BeamVertex);
    m_VertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
}

void CBeamSystem::Emit(const XMFLOAT3& start, const XMFLOAT3& end)
{
    m_Start = start;
    m_End = end;

    m_fLifeTime = m_fMaxLifeTime;
    m_bActive = true;
}

void CBeamSystem::Animate(float fTimeElapsed)
{
    if (!m_bActive) return;

    m_fLifeTime -= fTimeElapsed;

    if (m_fLifeTime <= 0.0f)
    {
        m_bActive = false;
        m_fLifeTime = 0.0f;
    }
}

void CBeamSystem::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    if (!m_bActive) return;
    if (!m_pShader) return;
    if (!m_pMappedVertices) return;

    XMVECTOR vStart = XMLoadFloat3(&m_Start);
    XMVECTOR vEnd = XMLoadFloat3(&m_End);

    XMVECTOR vBeam = XMVectorSubtract(vEnd, vStart);
    float length = XMVectorGetX(XMVector3Length(vBeam));

    if (length <= 0.001f) return;

    XMVECTOR vDir = XMVector3Normalize(vBeam);

    XMFLOAT3 camPos = pCamera->GetPosition();
    XMVECTOR vCameraPos = XMLoadFloat3(&camPos);
    XMVECTOR vMid = XMVectorScale(XMVectorAdd(vStart, vEnd), 0.5f);
    XMVECTOR vToCamera = XMVector3Normalize(XMVectorSubtract(vCameraPos, vMid));

    XMVECTOR vRight = XMVector3Cross(vToCamera, vDir);

    if (XMVectorGetX(XMVector3Length(vRight)) <= 0.001f)
        vRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    vRight = XMVector3Normalize(vRight);

    float alpha = m_fLifeTime / m_fMaxLifeTime;
    float halfWidth = m_fWidth * alpha;

    XMVECTOR p0 = XMVectorSubtract(vStart, XMVectorScale(vRight, halfWidth));
    XMVECTOR p1 = XMVectorAdd(vStart, XMVectorScale(vRight, halfWidth));
    XMVECTOR p2 = XMVectorSubtract(vEnd, XMVectorScale(vRight, halfWidth));
    XMVECTOR p3 = XMVectorAdd(vEnd, XMVectorScale(vRight, halfWidth));

    XMStoreFloat3(&m_pMappedVertices[0].m_xmf3Position, p0);
    m_pMappedVertices[0].m_xmf2TexCoord = XMFLOAT2(0.0f, 0.0f);

    XMStoreFloat3(&m_pMappedVertices[1].m_xmf3Position, p1);
    m_pMappedVertices[1].m_xmf2TexCoord = XMFLOAT2(0.0f, 1.0f);

    XMStoreFloat3(&m_pMappedVertices[2].m_xmf3Position, p2);
    m_pMappedVertices[2].m_xmf2TexCoord = XMFLOAT2(1.0f, 0.0f);

    XMStoreFloat3(&m_pMappedVertices[3].m_xmf3Position, p2);
    m_pMappedVertices[3].m_xmf2TexCoord = XMFLOAT2(1.0f, 0.0f);

    XMStoreFloat3(&m_pMappedVertices[4].m_xmf3Position, p1);
    m_pMappedVertices[4].m_xmf2TexCoord = XMFLOAT2(0.0f, 1.0f);

    XMStoreFloat3(&m_pMappedVertices[5].m_xmf3Position, p3);
    m_pMappedVertices[5].m_xmf2TexCoord = XMFLOAT2(1.0f, 1.0f);

    m_pShader->OnPrepareRender(pd3dCommandList);

    pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    pd3dCommandList->DrawInstanced(6, 1, 0, 0);
    ++g_nDrawCallCount;
}