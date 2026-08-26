#include "CFireballShader.h"

CFireballShader::CFireballShader()
{
}

CFireballShader::~CFireballShader()
{
}

D3D12_INPUT_LAYOUT_DESC CFireballShader::CreateInputLayout()
{
    // CCubeShader와 동일: POSITION(float3) + TEXCOORD(float2)
    // CTexturedVertex 레이아웃과 1:1 대응
    UINT nInputElementDescs = 2;
    D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

    pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    pd3dInputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

    D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
    d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
    d3dInputLayoutDesc.NumElements = nInputElementDescs;

    return d3dInputLayoutDesc;
}

D3D12_BLEND_DESC CFireballShader::CreateBlendState()
{
    D3D12_BLEND_DESC d3dBlendDesc;
    ::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));

    d3dBlendDesc.AlphaToCoverageEnable = FALSE;
    d3dBlendDesc.IndependentBlendEnable = FALSE;

    // 가산 블렌딩(Additive): DestBlend = ONE
    // 불꽃이 겹칠수록 밝아지는 발광 효과
    d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
    d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
    d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    return d3dBlendDesc;
}

D3D12_DEPTH_STENCIL_DESC CFireballShader::CreateDepthStencilState()
{
    D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
    ::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));

    // 깊이 테스트는 수행하되 쓰기는 하지 않음
    // 반투명 파티클끼리 앞뒤 관계 없이 정상 렌더링
    d3dDepthStencilDesc.DepthEnable = TRUE;
    d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 쓰기 OFF
    d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    d3dDepthStencilDesc.StencilEnable = FALSE;

    return d3dDepthStencilDesc;
}

D3D12_SHADER_BYTECODE CFireballShader::CreateVertexShader()
{
    return CompileShaderFromFile(L"Shaders.hlsl", "VSFireball", "vs_5_1", &m_pd3dVertexShaderBlob);
}

D3D12_SHADER_BYTECODE CFireballShader::CreatePixelShader()
{
    return CompileShaderFromFile(L"Shaders.hlsl", "PSFireball", "ps_5_1", &m_pd3dPixelShaderBlob);
}

D3D12_SHADER_BYTECODE CGreenSpiritShader::CreateVertexShader()
{
    return CompileShaderFromFile(L"Shaders.hlsl", "VSFireball", "vs_5_1", &m_pd3dVertexShaderBlob);
}

D3D12_SHADER_BYTECODE CGreenSpiritShader::CreatePixelShader()
{

    return CompileShaderFromFile(L"Shaders.hlsl", "PSGreenSpirit", "ps_5_1", &m_pd3dPixelShaderBlob);
}

D3D12_SHADER_BYTECODE CHitSparkShader::CreateVertexShader()
{
    return CompileShaderFromFile(L"Shaders.hlsl", "VSFireball", "vs_5_1", &m_pd3dVertexShaderBlob);
}

D3D12_SHADER_BYTECODE CHitSparkShader::CreatePixelShader()
{
    return CompileShaderFromFile(L"Shaders.hlsl", "PSHitSpark", "ps_5_1", &m_pd3dPixelShaderBlob);
}